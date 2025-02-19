/*
 * Copyright (C) 2014, 2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

WebInspector.TypeTokenAnnotator = function(sourceCodeTextEditor, script)
{
    WebInspector.Annotator.call(this, sourceCodeTextEditor);

    this._script = script;
    this._typeTokenNodes = [];
    this._typeTokenBookmarks = [];
};

WebInspector.TypeTokenAnnotator.prototype = {
    constructor: WebInspector.TypeTokenAnnotator,
    __proto__: WebInspector.Annotator.prototype,

    // Protected

    insertAnnotations: function()
    {
        if (!this.isActive())
            return;

        var scriptSyntaxTree = this._script.scriptSyntaxTree;

        if (!scriptSyntaxTree) {
            this._script.requestScriptSyntaxTree(function(syntaxTree) {
                // After requesting the tree, we still might get a null tree from a parse error.
                if (syntaxTree)
                    this.insertAnnotations();
            }.bind(this));

            return;
        }

        if (!scriptSyntaxTree.parsedSuccessfully)
            return;

        var {startOffset, endOffset} = this.sourceCodeTextEditor.visibleRangeOffsets();

        var startTime = Date.now();
        var allNodesInRange = scriptSyntaxTree.filterByRange(startOffset, endOffset);
        scriptSyntaxTree.updateTypes(allNodesInRange, function afterTypeUpdates() {
            // Because this is an asynchronous call, we could have been deactivated before the callback function is called.
            if (!this.isActive())
                return;

            allNodesInRange.forEach(this._insertTypeTokensForEachNode, this);

            var totalTime = Date.now() - startTime;
            var timeoutTime = Math.min(Math.max(7500, totalTime), 8 * totalTime);

            this._timeoutIdentifier = setTimeout(function timeoutUpdate() {
                this._timeoutIdentifier = null;
                this.insertAnnotations();
            }.bind(this), timeoutTime);
        }.bind(this));
    },

    clearAnnotations: function()
    {
        this._clearTypeTokens();
    },

    // Private

    _insertTypeTokensForEachNode: function(node)
    {
        var scriptSyntaxTree = this._script._scriptSyntaxTree;

        switch (node.type) {
        case WebInspector.ScriptSyntaxTree.NodeType.FunctionDeclaration:
        case WebInspector.ScriptSyntaxTree.NodeType.FunctionExpression:
            for (var param of node.params) {
                if (!param.attachments.__typeToken && param.attachments.types && param.attachments.types.isValid)
                    this._insertToken(param.range[0], param, false, WebInspector.TypeTokenView.TitleType.Variable, param.name);

                if (param.attachments.__typeToken)
                    param.attachments.__typeToken.update(param.attachments.types);
            }

            // If a function does not have an explicit return type, then don't show a return type unless we think it's a constructor.
            var functionReturnType = node.attachments.returnTypes;
            if (node.attachments.__typeToken || !functionReturnType || !functionReturnType.isValid)
                break;

            if (scriptSyntaxTree.containsNonEmptyReturnStatement(node.body) || !WebInspector.TypeSet.fromPayload(functionReturnType).isContainedIn(WebInspector.TypeSet.TypeBit.Undefined)) {
                var functionName = node.id ? node.id.name : null;
                this._insertToken(node.isGetterOrSetter ? node.getterOrSetterRange[0] : node.range[0], node, true, WebInspector.TypeTokenView.TitleType.ReturnStatement, functionName);
            }

            if (node.attachments.__typeToken)
                node.attachments.__typeToken.update(node.attachments.returnTypes);

            break;
        case WebInspector.ScriptSyntaxTree.NodeType.VariableDeclarator:
            var identifiers = scriptSyntaxTree.gatherIdentifiersInVariableDeclaration(node);
            for (identifier of identifiers) {
                if (!identifier.attachments.__typeToken && identifier.attachments.types && identifier.attachments.types.isValid)
                    this._insertToken(identifier.range[0], identifier, false, WebInspector.TypeTokenView.TitleType.Variable, identifier.name);

                if (identifier.attachments.__typeToken)
                    identifier.attachments.__typeToken.update(identifier.attachments.types);
            }

            break;
        }
    },

    _insertToken: function(originalOffset, node, shouldTranslateOffsetToAfterParameterList, typeTokenTitleType, functionOrVariableName)
    {
        var tokenPosition = this.sourceCodeTextEditor.originalOffsetToCurrentPosition(originalOffset);
        var currentOffset = this.sourceCodeTextEditor.currentPositionToCurrentOffset(tokenPosition);
        var sourceString = this.sourceCodeTextEditor.string;

        if (shouldTranslateOffsetToAfterParameterList) {
            // Translate the position to the closing parenthesis of the function arguments:
            // translate from: [type-token] function foo() {} => to: function foo() [type-token] {}
            currentOffset = this._translateToOffsetAfterFunctionParameterList(node, currentOffset, sourceString);
            tokenPosition = this.sourceCodeTextEditor.currentOffsetToCurrentPosition(currentOffset);
        }

        // Note: bookmarks render to the left of the character they're being displayed next to.
        // This is why right margin checks the current offset. And this is okay to do because JavaScript can't be written right-to-left.
        var isSpaceRegexp = /\s/;
        var shouldHaveLeftMargin = currentOffset !== 0 && !isSpaceRegexp.test(sourceString[currentOffset - 1]);
        var shouldHaveRightMargin = !isSpaceRegexp.test(sourceString[currentOffset]);
        var typeToken = new WebInspector.TypeTokenView(this, shouldHaveRightMargin, shouldHaveLeftMargin, typeTokenTitleType, functionOrVariableName);
        var bookmark = this.sourceCodeTextEditor.setInlineWidget(tokenPosition, typeToken.element);
        node.attachments.__typeToken = typeToken;
        this._typeTokenNodes.push(node);
        this._typeTokenBookmarks.push(bookmark);
    },

    _translateToOffsetAfterFunctionParameterList: function(node, offset, sourceString)
    {
        // The assumption here is that we get the offset starting at the function keyword (or after the get/set keywords).
        // We will return the offset for the closing parenthesis in the function declaration.
        // All this code is just a way to find this parenthesis while ignoring comments.

        var isMultiLineComment = false;
        var isSingleLineComment = false;
        var shouldIgnore = false;

        function isLineTerminator(char)
        {
            // Reference EcmaScript 5 grammar for single line comments and line terminators:
            // http://www.ecma-international.org/ecma-262/5.1/#sec-7.3
            // http://www.ecma-international.org/ecma-262/5.1/#sec-7.4
            return char === "\n" || char === "\r" || char === "\u2028" || char === "\u2029";
        }

        while ((sourceString[offset] !== ")" || shouldIgnore) && offset < sourceString.length) {
            if (isSingleLineComment && isLineTerminator(sourceString[offset])) {
                isSingleLineComment = false;
                shouldIgnore = false;
            } else if (isMultiLineComment && sourceString[offset] === "*" && sourceString[offset + 1] === "/") {
                isMultiLineComment = false;
                shouldIgnore = false;
                offset++;
            } else if (!shouldIgnore && sourceString[offset] === "/") {
                offset++;
                if (sourceString[offset] === "*")
                    isMultiLineComment = true;
                else if (sourceString[offset] === "/")
                    isSingleLineComment = true;
                else
                    throw new Error("Bad parsing. Couldn't parse comment preamble.");
                shouldIgnore = true;
            }

            offset++;
        }

        return offset + 1;
    },

    _clearTypeTokens: function()
    {
        this._typeTokenNodes.forEach(function(node) {
            node.attachments.__typeToken = null;
        });
        this._typeTokenBookmarks.forEach(function(bookmark) {
            bookmark.clear();
        });

        this._typeTokenNodes = [];
        this._typeTokenBookmarks = [];
    }
};
