/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "AccessibilityTableCell.h"

#include "AXObjectCache.h"
#include "AccessibilityTable.h"
#include "AccessibilityTableRow.h"
#include "ElementIterator.h"
#include "HTMLElement.h"
#include "HTMLNames.h"
#include "RenderObject.h"
#include "RenderTableCell.h"

namespace WebCore {
    
using namespace HTMLNames;

AccessibilityTableCell::AccessibilityTableCell(RenderObject* renderer)
    : AccessibilityRenderObject(renderer)
{
}

AccessibilityTableCell::~AccessibilityTableCell()
{
}

Ref<AccessibilityTableCell> AccessibilityTableCell::create(RenderObject* renderer)
{
    return adoptRef(*new AccessibilityTableCell(renderer));
}

bool AccessibilityTableCell::computeAccessibilityIsIgnored() const
{
    AccessibilityObjectInclusion decision = defaultObjectInclusion();
    if (decision == IncludeObject)
        return false;
    if (decision == IgnoreObject)
        return true;
    
    // Ignore anonymous table cells.
    if (!node())
        return true;
        
    if (!isTableCell())
        return AccessibilityRenderObject::computeAccessibilityIsIgnored();
    
    return false;
}

AccessibilityTable* AccessibilityTableCell::parentTable() const
{
    if (!is<RenderTableCell>(m_renderer))
        return nullptr;

    // If the document no longer exists, we might not have an axObjectCache.
    if (!axObjectCache())
        return nullptr;
    
    // Do not use getOrCreate. parentTable() can be called while the render tree is being modified 
    // by javascript, and creating a table element may try to access the render tree while in a bad state.
    // By using only get() implies that the AXTable must be created before AXTableCells. This should
    // always be the case when AT clients access a table.
    // https://bugs.webkit.org/show_bug.cgi?id=42652
    AccessibilityObject* parentTable = axObjectCache()->get(downcast<RenderTableCell>(*m_renderer).table());
    if (!is<AccessibilityTable>(parentTable))
        return nullptr;
    return downcast<AccessibilityTable>(parentTable);
}
    
bool AccessibilityTableCell::isTableCell() const
{
    // If the parent table is an accessibility table, then we are a table cell.
    // This used to check if the unignoredParent was a row, but that exploded performance if
    // this was in nested tables. This check should be just as good.
    AccessibilityObject* parentTable = this->parentTable();
    return is<AccessibilityTable>(parentTable) && downcast<AccessibilityTable>(*parentTable).isExposableThroughAccessibility();
}
    
AccessibilityRole AccessibilityTableCell::determineAccessibilityRole()
{
    // Always call determineAccessibleRole so that the ARIA role is set.
    // Even though this object reports a Cell role, the ARIA role will be used
    // to determine if it's a column header.
    AccessibilityRole defaultRole = AccessibilityRenderObject::determineAccessibilityRole();
#if !PLATFORM(EFL) && !PLATFORM(GTK)
    if (!isTableCell())
        return defaultRole;
    return CellRole;
#endif

    // If AccessibilityRenderObject::determineAccessibilityRole returns the type of CellRole,
    // which is derived from the role attribute, it does not change anything.
    if (defaultRole != UnknownRole) {
        AccessibilityRole ariaRole = ariaRoleAttribute();
        if (ariaRole == CellRole || ariaRole == RowHeaderRole || ariaRole == ColumnHeaderRole)
            return ariaRole;
    }

    // Here there is a more precide definition of the type of CellRole than was possible
    // at the level of AccessibilityRenderObject.
    if (defaultRole == ColumnHeaderRole || defaultRole == CellRole || defaultRole == RowHeaderRole) {
        if (isColumnHeaderCell())
            return ColumnHeaderRole;
        if (isRowHeaderCell())
            return RowHeaderRole;
        return CellRole;
    }
    return defaultRole;
}
    
bool AccessibilityTableCell::isTableHeaderCell() const
{
    return node() && node()->hasTagName(thTag);
}

bool AccessibilityTableCell::isColumnHeaderCell() const
{
    const AtomicString& scope = getAttribute(scopeAttr);
    if (scope == "col" || scope == "colgroup")
        return true;
    if (scope == "row" || scope == "rowgroup")
        return false;
    if (!isTableHeaderCell())
        return false;

    // We are in a situation after checking the scope attribute.
    // It is an attempt to resolve the type of th element without support in the specification.
    // Checking tableTag and tbodyTag allows to check the case of direct row placement in the table and lets stop the loop at the table level.
    for (Node* parentNode = node(); parentNode; parentNode = parentNode->parentNode()) {
        if (parentNode->hasTagName(theadTag))
            return true;
        if (parentNode->hasTagName(tfootTag))
            return false;
        if (parentNode->hasTagName(tableTag) || parentNode->hasTagName(tbodyTag)) {
            std::pair<unsigned, unsigned> rowRange;
            rowIndexRange(rowRange);
            if (!rowRange.first)
                return true;
            return false;
        }
    }
    return false;
}

bool AccessibilityTableCell::isRowHeaderCell() const
{
    const AtomicString& scope = getAttribute(scopeAttr);
    if (scope == "row" || scope == "rowgroup")
        return true;
    if (scope == "col" || scope == "colgroup")
        return false;
    if (!isTableHeaderCell())
        return false;

    // We are in a situation after checking the scope attribute.
    // It is an attempt to resolve the type of th element without support in the specification.
    // Checking tableTag allows to check the case of direct row placement in the table and lets stop the loop at the table level.
    for (Node* parentNode = node(); parentNode; parentNode = parentNode->parentNode()) {
        if (parentNode->hasTagName(tfootTag) || parentNode->hasTagName(tbodyTag) || parentNode->hasTagName(tableTag)) {
            std::pair<unsigned, unsigned> colRange;
            columnIndexRange(colRange);
            if (!colRange.first)
                return true;
            return false;
        }
        if (parentNode->hasTagName(theadTag))
            return false;
    }
    return false;
}

bool AccessibilityTableCell::isTableCellInSameRowGroup(AccessibilityTableCell* otherTableCell)
{
    Node* parentNode = node();
    for ( ; parentNode; parentNode = parentNode->parentNode()) {
        if (parentNode->hasTagName(theadTag) || parentNode->hasTagName(tbodyTag) || parentNode->hasTagName(tfootTag))
            break;
    }
    
    Node* otherParentNode = otherTableCell->node();
    for ( ; otherParentNode; otherParentNode = otherParentNode->parentNode()) {
        if (otherParentNode->hasTagName(theadTag) || otherParentNode->hasTagName(tbodyTag) || otherParentNode->hasTagName(tfootTag))
            break;
    }
    
    return otherParentNode == parentNode;
}


bool AccessibilityTableCell::isTableCellInSameColGroup(AccessibilityTableCell* tableCell)
{
    std::pair<unsigned, unsigned> colRange;
    columnIndexRange(colRange);
    
    std::pair<unsigned, unsigned> otherColRange;
    tableCell->columnIndexRange(otherColRange);
    
    if (colRange.first <= (otherColRange.first + otherColRange.second))
        return true;
    return false;
}
    
String AccessibilityTableCell::expandedTextValue() const
{
    return getAttribute(abbrAttr);
}
    
bool AccessibilityTableCell::supportsExpandedTextValue() const
{
    return isTableHeaderCell() && hasAttribute(abbrAttr);
}
    
void AccessibilityTableCell::columnHeaders(AccessibilityChildrenVector& headers)
{
    AccessibilityTable* parent = parentTable();
    if (!parent)
        return;

    // Choose columnHeaders as the place where the "headers" attribute is reported.
    ariaElementsFromAttribute(headers, headersAttr);
    // If the headers attribute returned valid values, then do not further search for column headers.
    if (!headers.isEmpty())
        return;
    
    std::pair<unsigned, unsigned> rowRange;
    rowIndexRange(rowRange);
    
    std::pair<unsigned, unsigned> colRange;
    columnIndexRange(colRange);
    
    for (unsigned row = 0; row < rowRange.first; row++) {
        AccessibilityTableCell* tableCell = parent->cellForColumnAndRow(colRange.first, row);
        if (!tableCell || tableCell == this || headers.contains(tableCell))
            continue;

        std::pair<unsigned, unsigned> childRowRange;
        tableCell->rowIndexRange(childRowRange);
            
        const AtomicString& scope = tableCell->getAttribute(scopeAttr);
        if (scope == "colgroup" && isTableCellInSameColGroup(tableCell))
            headers.append(tableCell);
        else if (tableCell->isColumnHeaderCell())
            headers.append(tableCell);
    }
}
    
void AccessibilityTableCell::rowHeaders(AccessibilityChildrenVector& headers)
{
    AccessibilityTable* parent = parentTable();
    if (!parent)
        return;

    std::pair<unsigned, unsigned> rowRange;
    rowIndexRange(rowRange);

    std::pair<unsigned, unsigned> colRange;
    columnIndexRange(colRange);

    for (unsigned column = 0; column < colRange.first; column++) {
        AccessibilityTableCell* tableCell = parent->cellForColumnAndRow(column, rowRange.first);
        if (!tableCell || tableCell == this || headers.contains(tableCell))
            continue;
        
        const AtomicString& scope = tableCell->getAttribute(scopeAttr);
        if (scope == "rowgroup" && isTableCellInSameRowGroup(tableCell))
            headers.append(tableCell);
        else if (tableCell->isRowHeaderCell())
            headers.append(tableCell);
    }
}
    
void AccessibilityTableCell::rowIndexRange(std::pair<unsigned, unsigned>& rowRange) const
{
    if (!is<RenderTableCell>(m_renderer))
        return;
    
    RenderTableCell& renderCell = downcast<RenderTableCell>(*m_renderer);
    rowRange.first = renderCell.rowIndex();
    rowRange.second = renderCell.rowSpan();
    
    // since our table might have multiple sections, we have to offset our row appropriately
    RenderTableSection* section = renderCell.section();
    RenderTable* table = renderCell.table();
    if (!table || !section)
        return;

    RenderTableSection* footerSection = table->footer();
    unsigned rowOffset = 0;
    for (RenderTableSection* tableSection = table->topSection(); tableSection; tableSection = table->sectionBelow(tableSection, SkipEmptySections)) {
        // Don't add row offsets for bottom sections that are placed in before the body section.
        if (tableSection == footerSection)
            continue;
        if (tableSection == section)
            break;
        rowOffset += tableSection->numRows();
    }

    rowRange.first += rowOffset;
}
    
void AccessibilityTableCell::columnIndexRange(std::pair<unsigned, unsigned>& columnRange) const
{
    if (!is<RenderTableCell>(m_renderer))
        return;
    
    const RenderTableCell& cell = downcast<RenderTableCell>(*m_renderer);
    columnRange.first = cell.table()->colToEffCol(cell.col());
    columnRange.second = cell.table()->colToEffCol(cell.col() + cell.colSpan()) - columnRange.first;
}
    
AccessibilityObject* AccessibilityTableCell::titleUIElement() const
{
    // Try to find if the first cell in this row is a <th>. If it is,
    // then it can act as the title ui element. (This is only in the
    // case when the table is not appearing as an AXTable.)
    if (isTableCell() || !is<RenderTableCell>(m_renderer))
        return nullptr;

    // Table cells that are th cannot have title ui elements, since by definition
    // they are title ui elements
    Node* node = m_renderer->node();
    if (node && node->hasTagName(thTag))
        return nullptr;
    
    RenderTableCell& renderCell = downcast<RenderTableCell>(*m_renderer);

    // If this cell is in the first column, there is no need to continue.
    int col = renderCell.col();
    if (!col)
        return nullptr;

    int row = renderCell.rowIndex();

    RenderTableSection* section = renderCell.section();
    if (!section)
        return nullptr;
    
    RenderTableCell* headerCell = section->primaryCellAt(row, 0);
    if (!headerCell || headerCell == &renderCell)
        return nullptr;

    if (!headerCell->element() || !headerCell->element()->hasTagName(thTag))
        return nullptr;
    
    return axObjectCache()->getOrCreate(headerCell);
}
    
} // namespace WebCore
