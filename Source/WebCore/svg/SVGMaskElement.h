/*
 * Copyright (C) 2005 Alexander Kellett <lypanov@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef SVGMaskElement_h
#define SVGMaskElement_h

#include "SVGAnimatedBoolean.h"
#include "SVGAnimatedEnumeration.h"
#include "SVGAnimatedLength.h"
#include "SVGElement.h"
#include "SVGExternalResourcesRequired.h"
#include "SVGNames.h"
#include "SVGTests.h"
#include "SVGUnitTypes.h"

namespace WebCore {
    
class RenderLayer;

class SVGMaskElement final : public SVGElement,
                             public SVGTests,
                             public SVGExternalResourcesRequired {
public:
    static Ref<SVGMaskElement> create(const QualifiedName&, Document&);

    void addClientRenderLayer(RenderLayer*);
    void removeClientRenderLayer(RenderLayer*);

private:
    SVGMaskElement(const QualifiedName&, Document&);

    virtual bool isValid() const override { return SVGTests::isValid(); }
    virtual bool needsPendingResourceHandling() const override { return false; }

    bool isSupportedAttribute(const QualifiedName&);
    virtual void parseAttribute(const QualifiedName&, const AtomicString&) override;
    virtual void svgAttributeChanged(const QualifiedName&) override;
    virtual void childrenChanged(const ChildChange&) override;

    HashSet<RenderLayer*> m_clientLayers;

    virtual RenderPtr<RenderElement> createElementRenderer(Ref<RenderStyle>&&) override;

    virtual bool selfHasRelativeLengths() const override { return true; }

    BEGIN_DECLARE_ANIMATED_PROPERTIES(SVGMaskElement)
        DECLARE_ANIMATED_ENUMERATION(MaskUnits, maskUnits, SVGUnitTypes::SVGUnitType)
        DECLARE_ANIMATED_ENUMERATION(MaskContentUnits, maskContentUnits, SVGUnitTypes::SVGUnitType)
        DECLARE_ANIMATED_LENGTH(X, x)
        DECLARE_ANIMATED_LENGTH(Y, y)
        DECLARE_ANIMATED_LENGTH(Width, width)
        DECLARE_ANIMATED_LENGTH(Height, height)
        DECLARE_ANIMATED_BOOLEAN(ExternalResourcesRequired, externalResourcesRequired)
    END_DECLARE_ANIMATED_PROPERTIES

    // SVGTests
    virtual void synchronizeRequiredFeatures() override { SVGTests::synchronizeRequiredFeatures(this); }
    virtual void synchronizeRequiredExtensions() override { SVGTests::synchronizeRequiredExtensions(this); }
    virtual void synchronizeSystemLanguage() override { SVGTests::synchronizeSystemLanguage(this); }
};

}

#endif
