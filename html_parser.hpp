/*
 * Original Version Copyright (c) 2017 SPLI (rangerlee@foxmail.com)
 * Latest version available at: http://github.com/rangerlee/htmlparser.git
 *
 * A Simple html parser.
 * More information can get from README.md
 *
 *
 * Changed Version Copyright (c) 2025 DSI (jtaylor@jtdata.com)
 * Latest version available at: https://github.com/jtaylorme/html_parser
 *
 * Version 1.0.3
 */

#ifndef HTMLPARSER_HPP_
#define HTMLPARSER_HPP_

#include <string>
#include <stdio.h>
#include <iostream>
#include <cstring>
#include <vector>
#include <map>
#include <set>
#include <unordered_set>
#include <memory>
#include <algorithm>   // std::transform
#include <sstream>     // std::wistringstream, std::wostringstream
#include <cwctype>     // std::towlower
#include <cwchar>      // wcsncmp, wcslen

using std::enable_shared_from_this;
using std::shared_ptr;
using std::weak_ptr;


 

inline std::wstring toLower(const std::wstring& str);
inline std::vector<std::wstring> TokenizeXPath(const std::wstring& input);
inline std::wstring EscapeForXPath(const std::wstring& value);
inline static bool EqualIgnoreCase(const std::wstring& a, const std::wstring& b);
inline static bool StartsWith(const std::wstring& s, const std::wstring& prefix);
inline static bool EndsWith(const std::wstring& s, const std::wstring& suffix);
inline std::wstring Trim(const std::wstring& str);
inline bool AttrContains(const std::map<std::wstring, std::wstring>& attrs, const std::wstring& name, const std::wstring& substring);
inline bool AttrStartsWith(const std::map<std::wstring, std::wstring>& attrs, const std::wstring& name, const std::wstring& prefix);
inline bool AttrEndsWith(const std::map<std::wstring, std::wstring>& attrs, const std::wstring& name, const std::wstring& suffix);
inline bool ClassStartsWith(const std::vector<std::wstring>& classlist, const std::wstring& prefix);
inline bool ClassEndsWith(const std::vector<std::wstring>& classlist, const std::wstring& suffix);
inline bool ClassContains(const std::vector<std::wstring>& classlist, const std::wstring& contains);
inline std::wstring ClearQuotes(std::wstring val);


/**
 * class HtmlElement
 * HTML Element struct
 */
class HtmlElement : public enable_shared_from_this<HtmlElement> {
public:
    friend class HtmlParser;

    friend class HtmlDocument;

public:
    /**
     * for children traversals.
     */
    typedef std::vector<shared_ptr<HtmlElement>>::const_iterator ChildIterator;

    ChildIterator ChildBegin() const { return children.cbegin(); }
    ChildIterator ChildEnd()   const { return children.cend(); }

    /**
     * for attribute traversals.
     */
    typedef std::map<std::wstring, std::wstring>::const_iterator AttributeIterator;

    AttributeIterator AttributeBegin() const { return attribute.cbegin(); }
    AttributeIterator AttributeEnd()   const { return attribute.cend(); }

public:

    HtmlElement() {}

    HtmlElement(shared_ptr<HtmlElement> p)
        : parent(p) {
    }

    std::wstring GetAttribute(const std::wstring& k) {
        if (attribute.find(k) != attribute.end()) {
            return attribute[k];
        }
        return L"";
    }

    void SetAttribute(const std::wstring& j, const std::wstring& k) {
        if (k.empty()) {
            attribute.erase(j);
            if (j == L"class") classlist.clear();
        }
        else {
            attribute[j] = k;
            if (j == L"class") {
                classlist.clear();
                std::wistringstream iss(k);
                std::wstring token;
                while (iss >> token) {
                    classlist.push_back(token);
                }
            }
        }
    }


    std::map<std::wstring, std::wstring> GetAttributes() {

        return attribute;
    }

    shared_ptr<HtmlElement> GetElementById(const std::wstring& id)
    {
        for (HtmlElement::ChildIterator it = children.begin(); it != children.end(); ++it) {
            std::wstring gid = (*it)->GetAttribute(L"id");
            if ((*it)->GetAttribute(L"id") == id)
            {
                return *it;
            }
            shared_ptr<HtmlElement> r = (*it)->GetElementById(id);

            if (r) return r;
        }

        return shared_ptr<HtmlElement>();
    }

    std::vector<shared_ptr<HtmlElement> > GetElementsById(const std::wstring& id) {
        std::vector<shared_ptr<HtmlElement> > result;
        GetElementsById(id, result);
        return result;
    }


    std::vector<shared_ptr<HtmlElement> > GetElementsByClassName(const std::wstring& name, const std::wstring& tag = L"") {
        std::vector<shared_ptr<HtmlElement> > result;
        GetElementsByClassName(name, tag, result);
        return result;
    }

    // Inside HtmlElement class (public:)
    std::vector<std::wstring> GetClassList() const {
        return classlist;
    }

    bool HasClass(const std::wstring& cls) const {
        return std::find(classlist.begin(), classlist.end(), cls) != classlist.end();
    }

    void AddClass(const std::wstring& cls) {
        if (!HasClass(cls)) {
            classlist.push_back(cls);
            UpdateClassAttribute();
        }
    }

    void RemoveClass(const std::wstring& cls) {
        auto it = std::remove(classlist.begin(), classlist.end(), cls);
        if (it != classlist.end()) {
            classlist.erase(it, classlist.end());
            UpdateClassAttribute();
        }
    }

    void ToggleClass(const std::wstring& cls) {
        if (HasClass(cls))
            RemoveClass(cls);
        else
            AddClass(cls);
    }

    void ClearClasses() {
        classlist.clear();
        attribute.erase(L"class");
    }


    std::vector<shared_ptr<HtmlElement> > GetElementByTagName(const std::wstring& name) {
        std::vector<shared_ptr<HtmlElement> > result;
        GetElementByTagName(name, result);
        return result;
    }
 
    bool IsLowerAlphaNumeric(const std::wstring& str) {
        for (wchar_t c : str) {
            if (!((c >= L'a' && c <= L'z') || (c >= L'0' && c <= L'9')))
                return false;
        }
        return true;
    }

    bool IsLowerAlphaOnly(const std::wstring& str) {
        for (wchar_t ch : str) {
            if (ch < L'a' || ch > L'z')
                return false;
        }
        return !str.empty(); // optional: false if empty
    }

    // --- Entry Point ---
    void SelectElement(const std::wstring& rule,
        std::vector<std::shared_ptr<HtmlElement>>& result) {
        std::vector<std::wstring> ruleTokens = TokenizeXPath(rule);
      
       //***************************************************************************
       //If you need to debug. Uncomment below for a display of tokens.
       // std::wstring list = L""; for (size_t i = 0; i < ruleTokens.size(); i++)   { list += ruleTokens[i] + L"\n"; if (i == ruleTokens.size() - 1)  myMsg(L"tokens: " + std::to_wstring(ruleTokens.size()), list);   }
       //***************************************************************************
       
        // Enforce rigid structure
        if (ruleTokens.size() >= 2) ruleTokens[1] = toLower(ruleTokens[1]);
        if (ruleTokens[0] != L"/" && ruleTokens[0] != L"//") return;
        if (ruleTokens[1] != L"*" && !IsLowerAlphaNumeric(ruleTokens[1])) return;
        if (ruleTokens.size() >= 3 && ruleTokens[2] != L"[") return;
        if (ruleTokens.size() >= 3 && ruleTokens[2] == L"[" && ruleTokens.back() != L"]") return;
        if (ruleTokens.size() >= 4) ruleTokens[3] = toLower(ruleTokens[3]);
        if (ruleTokens.size() >= 4 && ruleTokens[3] != L"!" &&
            ruleTokens[3] != L"@" &&
            ruleTokens[3] != L"contains" &&
            ruleTokens[3] != L"text" &&
            ruleTokens[3] != L"starts-with" &&
            ruleTokens[3] != L"ends-with") return;
        this->SelectElement(ruleTokens, 0, result);
    }

    
    // --- Recursive selector ---
    bool SelectElement(const std::vector<std::wstring>& tokens,
        size_t idx,
        std::vector<std::shared_ptr<HtmlElement>>& results)
    {
        if (idx >= tokens.size()) return false;
        bool matched = false;
        const std::wstring& tok = tokens[idx];

        // "/" direct child
        if (tok == L"/") {
            for (auto& c : children)
                matched |= c->SelectElement(tokens, idx + 1, results);
            return matched;
        }


        // "//" descendant-or-self
        if (tok == L"//") {
            for (auto& c : children) {
                matched |= c->SelectElement(tokens, idx + 1, results); // next token
                matched |= c->SelectElement(tokens, idx, results);     // keep descending
            }
            return matched;
        }

        // Match tag or "*"
        if (tok == L"*" || EqualIgnoreCase(this->name, tok)) {
            size_t nextIdx = idx + 1;
           
            if (nextIdx < tokens.size() && tokens[nextIdx] == L"[") {
                size_t closeIdx = tokens.size() - 1; // rigid: last token must be "]"

                std::wstring condType = tokens[3]; // rigid structure: token 3

                bool condMatched = false;

                if (condType == L"@") {
                    if (tokens[5] == L"=") {
                        std::wstring name = tokens[4];
                        std::wstring val = tokens[6];
                        val = ClearQuotes(val);

                        auto it = attribute.find(name);
                        if (it != attribute.end()) {
                            if (name == L"class") {
                                condMatched = HasClass(val);
                            }
                            else {
                                condMatched = (it->second == val);
                            }
                        }
                    }
                    else if (tokens[5] == L"]")
                    {
                        std::wstring name = tokens[4];
                     
                        auto it = attribute.find(name);
                        if (it != attribute.end()) {
                            condMatched = true;
                        }
                    }
                }
                else if (condType == L"text") 
                {
                    condMatched = false;
                    if (tokens[6] == L",")
                    {
                        std::wstring name = Trim(tokens[5]);
                        std::wstring val = Trim(tokens[7]);
                        std::wstring text = this->text();
                        val = ClearQuotes(val);
                       // myMsg(name + L" - " + val, text);
                        if (name == L"equals")
                        {
                            if (val == text)
                                condMatched = true;
                        }
                        else if (name == L"contains")
                        {
                            if (text.find(val) != std::wstring::npos)
                                condMatched = true;
                        }
                        else if (name == L"starts-with")
                        {
                            if (StartsWith(text, val))
                                condMatched = true;
                        }
                        else if (name == L"ends-with")
                        {
                            if (EndsWith(text, val))
                            {
                                condMatched = true;
                            }
                        }
                    }
                }
                else if (condType == L"contains") {
                    if (tokens[7] == L",")
                    {
                        std::wstring name = Trim(tokens[6]); 
                        std::wstring val = Trim(tokens[8]);
                        val = ClearQuotes(val);
                        if (name == L"class") {
                            condMatched = ClassContains(classlist, val);   
                        }
                        else {
                            condMatched = AttrContains(attribute, name, val);
                        }
                    }
                }
                else if (condType == L"starts-with") {
                    if (tokens[7] == L",")
                    {
                        std::wstring name = Trim(tokens[6]);
                        std::wstring val = Trim(tokens[8]);
                        if (!val.empty() && val.front() == L'\'') val.erase(val.begin());
                        if (!val.empty() && val.back() == L'\'') val.pop_back();

                        if (name == L"class") {
                           condMatched = ClassStartsWith(classlist, val);  
                        }
                        else {
                            condMatched = AttrStartsWith(attribute, name, val);
                        }
                    }
                }
                else if (condType == L"ends-with") {
                    if (tokens[7] == L",")
                    {
                        std::wstring name = Trim(tokens[6]);
                        std::wstring val = Trim(tokens[8]);
                        if (!val.empty() && val.front() == L'\'') val.erase(val.begin());
                        if (!val.empty() && val.back() == L'\'') val.pop_back();

                        if (name == L"class") {
                            condMatched = ClassEndsWith(classlist, val);  
                        }
                        else {
                            condMatched = AttrEndsWith(attribute, name, val);
                        }
                    }
                }

                if (!condMatched) return false;

                // If this is the last token, push result
                if (closeIdx == tokens.size() - 1) {
                    results.push_back(shared_from_this());
                    return true;
                }

                // Else continue recursion after ]
                for (auto& c : children)
                    matched |= c->SelectElement(tokens, closeIdx + 1, results);
                return matched;
            }

            // No condition, check end of tokens
            if (nextIdx == tokens.size()) {
                results.push_back(shared_from_this());
                return true;
            }

            // Recurse to children
            for (auto& c : children)
                matched |= c->SelectElement(tokens, nextIdx, results);
            return matched;
        }

        return false;
    }

    //********************************************************************************

    shared_ptr<HtmlElement> GetParent() {
        return parent.lock();
    }

    shared_ptr<HtmlElement> GetSiblingNext() {

        shared_ptr<HtmlElement> el = shared_from_this();

        if (el->GetParent()) {
            auto& children = el->GetParent()->children;

            auto it = std::find(children.begin(), children.end(), el);

            if (it != children.end()) {
                if (it + 1 != children.end()) {
                    auto nextSibling = *(it + 1);
                    return nextSibling;
                }
            }
        }
        return nullptr;
    }


    std::vector<shared_ptr<HtmlElement>> GetChildren() {

        return children;

    }

    shared_ptr<HtmlElement> GetSiblingPrev() {

        shared_ptr<HtmlElement> el = shared_from_this();

        if (el->GetParent()) {
            auto& children = el->GetParent()->children;

            auto it = std::find(children.begin(), children.end(), el);

            if (it != children.end()) {
                if (it != children.begin()) {
                    auto prevSibling = *(it - 1);
                    return prevSibling;
                }
            }
        }
        return nullptr;
    }



    int SetInnerText(std::wstring text) {
        auto el = shared_from_this();

        if (el->children.empty()) {
            // Create a text node if none exists
            auto textNode = std::make_shared<HtmlElement>();
            textNode->value = text;
            el->children.push_back(textNode);

        }
        else {
            el->children[0]->value = text;
        }

        return 0;
    }

    int SetInnerHTML(std::shared_ptr<HtmlElement> tempRoot) {
        auto el = shared_from_this();

        // Clear current children
        el->children.clear();

        // Append parsed children to our element
        for (auto& child : tempRoot->GetChildren()) {
            // Make a new HtmlElement with the same data but correct parent
            child->parent = el;
            el->children.push_back(child);
        }

        return 0;
    }


    const std::wstring& GetValue() {
        if (value.empty() && children.size() == 1 && children[0]->GetName() == L"plain") {
            return children[0]->GetValue();
        }

        return value;
    }


    const std::wstring& GetName() {
        return name;
    }

    std::wstring text() {
        std::wstring str;
        PlainStylize(str);
        return str;
    }

    void PlainStylize(std::wstring& str) {
        if (name == L"head" || name == L"meta" || name == L"style" || name == L"script" || name == L"link") {
            return;
        }

        if (name == L"plain") {
            str.append(value);
            return;
        }

        for (size_t i = 0; i < children.size();) {
            children[i]->PlainStylize(str);

            if (++i < children.size()) {
                std::wstring ele = children[i]->GetName();
                if (ele == L"td") {
                    str.append(L"\t");
                }
                else if (ele == L"tr" || ele == L"br" || ele == L"div" || ele == L"p" || ele == L"hr" || ele == L"area" ||
                    ele == L"h1" || ele == L"h2" || ele == L"h3" || ele == L"h4" || ele == L"h5" || ele == L"h6" || ele == L"h7") {
                    str.append(L"\n");
                }
            }
        }
    }

    std::wstring OuterHTML() {
        std::wstring str;
        HtmlStylize(str);
        return str;
    }

    std::wstring InnerHTML() {
        std::wstring str;

        // Add inner text if there are no children
        if (children.empty()) {
            str.append(value);
        }
        else {
            for (size_t i = 0; i < children.size(); ++i) {
                children[i]->HtmlStylize(str);
            }
        }

        return str;
    }


    void HtmlStylize(std::wstring& str) {
        if (name.empty()) {
            for (size_t i = 0; i < children.size(); i++) {
                children[i]->HtmlStylize(str);
            }

            return;
        }
        else if (name == L"plain") {
            str.append(value);
            return;
        }

        str.append(L"<" + name);
        std::map<std::wstring, std::wstring>::const_iterator it = attribute.begin();
        for (; it != attribute.end(); it++) {
            str.append(L" " + it->first + L"=\"" + it->second + L"\"");
        }
        str.append(L">");

        if (children.empty()) {
            str.append(value);
        }
        else {
            for (size_t i = 0; i < children.size(); i++) {
                children[i]->HtmlStylize(str);
            }
        }

        str.append(L"</" + name + L">");
    }

private:


    void GetElementsByClassName(const std::wstring& cls, const std::wstring& tag, std::vector<std::shared_ptr<HtmlElement>>& result)
    {

        if (HasClass(cls))
        {
            if (tag != L"" && toLower(tag) == toLower(this->GetName()))
                InsertIfNotExists(result, shared_from_this());
            if (tag == L"")
                InsertIfNotExists(result, shared_from_this());
        }
        for (ChildIterator it = ChildBegin(); it != ChildEnd(); ++it) {
            (*it)->GetElementsByClassName(cls, tag, result);
        }
    }


    void GetElementsById(const std::wstring& id, std::vector<shared_ptr<HtmlElement> >& result) {
        std::wstring xpath = L"//*[@id='" + EscapeForXPath(id) + L"']";
        std::vector<std::wstring> ruleToken = TokenizeXPath(xpath);
        SelectElement(ruleToken, ruleToken.size(), result);
        return;
    }

    void GetElementByTagName(const std::wstring& name, std::vector<shared_ptr<HtmlElement>>& result) {
        for (HtmlElement::ChildIterator it = children.begin(); it != children.end(); ++it) {
            if (_wcsicmp((*it)->name.c_str(), name.c_str()) == 0)
                InsertIfNotExists(result, *it);
            
            (*it)->GetElementByTagName(name, result);
        }
    }


    void GetAllElement(std::vector<shared_ptr<HtmlElement> >& result) {
        for (size_t i = 0; i < children.size(); ++i) {
            InsertIfNotExists(result, children[i]);
            children[i]->GetAllElement(result);
        }
    }

    void Parse(const std::wstring& attr) {
        size_t index = 0;
        std::wstring k;
        std::wstring v;
        wchar_t split = L' ';
        bool quota = false;

        enum ParseAttrState {
            PARSE_ATTR_KEY,
            PARSE_ATTR_VALUE_BEGIN,
            PARSE_ATTR_VALUE_END,
        };

        ParseAttrState state = PARSE_ATTR_KEY;

        while (attr.size() > index) {
            wchar_t input = attr.at(index);
            switch (state) {
            case PARSE_ATTR_KEY: {
                if (input == L'\t' || input == L'\r' || input == L'\n') {
                }
                else if (input == L'\'' || input == L'"') {
                    std::wcerr << L"WARN : attribute unexpected " << input << std::endl;
                }
                else if (input == L' ') {
                    if (!k.empty()) {
                        attribute[k] = v;
                        k.clear();
                    }
                }
                else if (input == L'=') {
                    state = PARSE_ATTR_VALUE_BEGIN;
                }
                else {
                    k.append(attr.c_str() + index, 1);
                }
            }
                               break;

            case PARSE_ATTR_VALUE_BEGIN: {
                if (input == L'\t' || input == L'\r' || input == L'\n' || input == L' ') {
                    if (!k.empty()) {
                        attribute[k] = v;
                        k.clear();
                    }
                    state = PARSE_ATTR_KEY;
                }
                else if (input == L'\'' || input == L'"') {
                    split = input;
                    quota = true;
                    state = PARSE_ATTR_VALUE_END;
                }
                else {
                    v.append(attr.c_str() + index, 1);
                    quota = false;
                    state = PARSE_ATTR_VALUE_END;
                }
            }
                                       break;

            case PARSE_ATTR_VALUE_END: {
                if ((quota && input == split) || (!quota && (input == L'\t' || input == L'\r' || input == L'\n' || input == L' '))) {
                    attribute[k] = v;
                    k.clear();
                    v.clear();
                    state = PARSE_ATTR_KEY;
                }
                else {
                    v.append(attr.c_str() + index, 1);
                }
            }
                                     break;
            }

            index++;
        }

        if (!k.empty()) {
            attribute[k] = v;
        }

        //trim
        if (!value.empty()) {
            value.erase(0, value.find_first_not_of(L" "));
            value.erase(value.find_last_not_of(L" ") + 1);
        }

        // After parsing attributes into `attribute`
        auto it = attribute.find(L"class");
        if (it != attribute.end()) {
            classlist.clear();
            std::wistringstream iss(it->second);
            std::wstring token;
            while (iss >> token) {
                classlist.push_back(token);
            }
        }

    }


    static void InsertIfNotExists(std::vector<std::shared_ptr<HtmlElement>>& vec, const std::shared_ptr<HtmlElement>& ele) {
        for (size_t i = 0; i < vec.size(); i++) {
            if (vec[i] == ele) return;
        }

        vec.push_back(ele);
    }

    // Private helper to sync classlist attribute["class"]
private:
    void UpdateClassAttribute() {
        if (classlist.empty()) {
            attribute.erase(L"class");
            return;
        }
        std::wstring combined;
        for (size_t i = 0; i < classlist.size(); ++i) {
            if (i > 0) combined += L" ";
            combined += classlist[i];
        }
        attribute[L"class"] = combined;
    }
private:
    std::wstring name;
    std::wstring value;
    std::map<std::wstring, std::wstring> attribute;
    std::vector<std::wstring> classlist;
    weak_ptr<HtmlElement> parent;
    std::vector<shared_ptr<HtmlElement> > children;
};

/**
 * class HtmlDocument
 * Html Doc struct
 */
class HtmlDocument {
public:
    HtmlDocument(shared_ptr<HtmlElement>& root)
        : root_(root) {
    }

    std::shared_ptr<HtmlElement> GetRoot() {
        return root_;
    }
    shared_ptr<HtmlElement> GetElementById(const std::wstring& id) {
        return root_->GetElementById(id);
    }

    std::vector<shared_ptr<HtmlElement> >  GetElementsById(const std::wstring& id) {
        return root_->GetElementsById(id);
    }

    std::vector<shared_ptr<HtmlElement> > GetElementsByClassName(const std::wstring& name) {
        return root_->GetElementsByClassName(name);
    }
    std::vector<shared_ptr<HtmlElement> > GetElementByTagName(const std::wstring& name) {
        return root_->GetElementByTagName(name);
    }

    void SelectElement(const std::wstring& rule, std::vector<std::shared_ptr<HtmlElement>>& result) {
        std::vector<std::wstring> ruleToken = TokenizeXPath(rule);
        this->SelectElement(ruleToken, 0, result);
    }

 
    std::vector<shared_ptr<HtmlElement> > SelectElement(std::vector<std::wstring> ruleToken, size_t rtSize, std::vector<shared_ptr<HtmlElement>>& result) {
        HtmlElement::ChildIterator it = root_->ChildBegin();
        for (; it != root_->ChildEnd(); it++) {
            (*it)->SelectElement(ruleToken, rtSize, result);
        }

        return result;
    }

    std::wstring OuterHTML() {
        return root_->OuterHTML();
    }
    std::wstring InnerHTML() {
        return root_->InnerHTML();
    }
    std::wstring text() {
        return root_->text();
    }

private:
    shared_ptr<HtmlElement> root_;
};

/**
 * class HtmlParser
 * html parser and only one interface
 */
class HtmlParser {
public:
    HtmlParser() {
        static const std::wstring token[] = { L"br", L"hr", L"img", L"input", L"link", L"meta",
        L"area", L"base", L"col", L"command", L"embed", L"keygen", L"param", L"source", L"track", L"wbr" };
        self_closing_tags_.insert(token, token + sizeof(token) / sizeof(token[0]));
    }

    /**
     * parse html by C-Style data
     * @param data
     * @param len
     * @return html document object
     */
    shared_ptr<HtmlDocument> Parse(const wchar_t* data, size_t len) {
        stream_ = data;
        length_ = len;
        size_t index = 0;
        root_.reset(new HtmlElement());
        while (length_ > index) {
            wchar_t input = stream_[index];
            if (input == L'\r' || input == L'\n' || input == L'\t' || input == L' ') {
                index++;
            }
            else if (input == L'<') {
                index = ParseElement(index, root_);
            }
            else {
                break;
            }
        }

        return shared_ptr<HtmlDocument>(new HtmlDocument(root_));
    }

    /**
     * parse html by string data
     * @param data
     * @return html document object
     */
    shared_ptr<HtmlDocument> Parse(const std::wstring& data) {
        return Parse(data.data(), data.size());
    }

private:
    size_t ParseElement(size_t index, shared_ptr<HtmlElement>& element) {
        while (length_ > index) {
            wchar_t input = stream_[index + 1];
            if (input == L'!') {
                if (wcsncmp(stream_ + index, L"<!--", 4) == 0) {
                    return SkipUntil(index + 2, L"-->");
                }
                else {
                    return SkipUntil(index + 2, L'>');
                }
            }
            else if (input == L'/') {
                return SkipUntil(index, L'>');
            }
            else if (input == L'?') {
                return SkipUntil(index, L"?>");
            }

            shared_ptr<HtmlElement> self(new HtmlElement(element));

            enum ParseElementState {
                PARSE_ELEMENT_TAG,
                PARSE_ELEMENT_ATTR,
                PARSE_ELEMENT_VALUE,
                PARSE_ELEMENT_TAG_END
            };

            ParseElementState state = PARSE_ELEMENT_TAG;
            index++;
            wchar_t split = 0;
            std::wstring attr;

            while (length_ > index) {
                switch (state) {
                case PARSE_ELEMENT_TAG: {
                    wchar_t input = stream_[index];
                    if (input == L' ' || input == L'\r' || input == L'\n' || input == L'\t') {
                        if (!self->name.empty()) {
                            state = PARSE_ELEMENT_ATTR;
                        }
                        index++;
                    }
                    else if (input == L'/') {
                        self->Parse(attr);
                        element->children.push_back(self);
                        return SkipUntil(index, L'>');
                    }
                    else if (input == L'>') {
                        if (self_closing_tags_.find(self->name) != self_closing_tags_.end()) {
                            element->children.push_back(self);
                            return ++index;
                        }
                        state = PARSE_ELEMENT_VALUE;
                        index++;
                    }
                    else {
                        self->name.append(stream_ + index, 1);
                        index++;
                    }
                }
                                      break;

                case PARSE_ELEMENT_ATTR: {
                    wchar_t input = stream_[index];
                    if (input == L'>') {
                        if (stream_[index - 1] == L'/') {
                            attr.erase(attr.size() - 1);
                            self->Parse(attr);
                            element->children.push_back(self);
                            return ++index;
                        }
                        else if (self_closing_tags_.find(self->name) != self_closing_tags_.end()) {
                            self->Parse(attr);
                            element->children.push_back(self);
                            return ++index;
                        }
                        state = PARSE_ELEMENT_VALUE;
                        index++;
                    }
                    else {
                        attr.append(stream_ + index, 1);
                        index++;
                    }
                }
                                       break;

                case PARSE_ELEMENT_VALUE: {
                    if (self->name == L"script" || self->name == L"noscript" || self->name == L"style") {
                        std::wstring close = L"</" + self->name + L">";

                        size_t pre = index;
                        index = SkipUntil(index, close.c_str());
                        if (index > (pre + close.size()))
                            self->value.append(stream_ + pre, index - pre - close.size());

                        self->Parse(attr);
                        element->children.push_back(self);
                        return index;
                    }

                    wchar_t input = stream_[index];
                    if (input == L'<') {
                        if (!self->value.empty()) {
                            shared_ptr<HtmlElement> child(new HtmlElement(self));
                            child->name = L"plain";
                            child->value.swap(self->value);
                            self->children.push_back(child);
                        }

                        if (stream_[index + 1] == L'/') {
                            state = PARSE_ELEMENT_TAG_END;
                        }
                        else {
                            index = ParseElement(index, self);
                        }
                    }
                    else if (input != L'\r' && input != L'\n' && input != L'\t') {
                        self->value.append(stream_ + index, 1);
                        index++;
                    }
                    else {
                        index++;
                    }
                }
                                        break;

                case PARSE_ELEMENT_TAG_END:
                {
                    index += 2; // skip "</"

                    // Read tag name only (stop at space, tab, newline, or '>')
                    size_t nameStart = index;
                    while (length_ > index && stream_[index] != L'>' &&
                        stream_[index] != L' ' && stream_[index] != L'\t' &&
                        stream_[index] != L'\r' && stream_[index] != L'\n')
                    {
                        index++;
                    }
                    std::wstring closeTag(stream_ + nameStart, index - nameStart);

                    // Skip any whitespace before '>'
                    while (length_ > index && (stream_[index] == L' ' || stream_[index] == L'\t' ||
                        stream_[index] == L'\r' || stream_[index] == L'\n'))
                    {
                        index++;
                    }

                    // Expect '>' to end the closing tag
                    if (length_ > index && stream_[index] == L'>') {
                        index++; // move past '>'
                    }

                    if (toLower(closeTag) == toLower(self->name)) {
                        // Correct closing tag for this element
                        self->Parse(attr);
                        element->children.push_back(self);
                        return index;
                    }
                    else {
                        // Check if this closing tag actually belongs to a parent
                        shared_ptr<HtmlElement> parent = self->GetParent();
                        while (parent) {
                            if (toLower(parent->name) == toLower(closeTag)) {
                                std::wcerr << L"WARN : element not closed <" << self->name << L">" << std::endl;
                                self->Parse(attr);
                                element->children.push_back(self);
                                return nameStart - 2; // rewind to before "</"
                            }
                            parent = parent->GetParent();
                        }

                        // Unexpected closing tag
                        std::wcerr << L"WARN : unexpected closed element </" << closeTag
                            << L"> for <" << self->name << L">" << std::endl;
                        state = PARSE_ELEMENT_VALUE;
                    }
                }
                break;

                break;

                break;
                }
            }
        }

        return index;
    }

    size_t SkipUntil(size_t index, const wchar_t* data) {
        while (length_ > index) {
            if (wcsncmp(stream_ + index, data, wcslen(data)) == 0) {
                return index + wcslen(data);
            }
            else {
                index++;
            }
        }

        return index;
    }

    size_t SkipUntil(size_t index, const wchar_t data) {
        while (length_ > index) {
            if (stream_[index] == data) {
                return ++index;
            }
            else {
                index++;
            }
        }

        return index;
    }

private:
    const wchar_t* stream_;
    size_t length_;
    std::set<std::wstring> self_closing_tags_;
    shared_ptr<HtmlElement> root_;
};

static std::wstring toLower(const std::wstring& str)
{
    std::wstring lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(),
        [](wchar_t c) -> wchar_t { return static_cast<wchar_t>(std::towlower(c)); });
    return lowerStr;
}




inline std::wstring EscapeForXPath(const std::wstring& value)
{
    // XPath doesn't allow unescaped single quotes inside single-quoted strings.
    // So we switch to using concat() in such cases.
    if (value.find('\'') == std::wstring::npos) {
        return value;  // safe to embed directly
    }

    std::wostringstream oss;
    oss << L"concat(";
    bool first = true;
    std::wistringstream ss(value);
    std::wstring part;
    while (std::getline(ss, part, L'\'')) {
        if (!first) oss << L", \"'\", ";
        oss << L"'" << part << L"'";
        first = false;
    }
    oss << L")";
    return oss.str();
}

 
#include <cctype>

std::vector<std::wstring> TokenizeXPath(const std::wstring& input) {
    std::vector<std::wstring> tokens;
    size_t i = 0;
    size_t len = input.size();

    auto isOperatorChar = [](wchar_t c) {
        return (c == '=' || c == '!' || c == '<' || c == '>');
        };

    while (i < len) {
        wchar_t c = input[i];

        // Skip whitespace
        if (std::isspace(c)) {
            ++i;
            continue;
        }

        // Double slash "//"
        if (c == '/' && i + 1 < len && input[i + 1] == '/') {
            tokens.push_back(L"//");
            i += 2;
            continue;
        }

        // "::" axis operator
        if (c == ':' && i + 1 < len && input[i + 1] == ':') {
            tokens.push_back(L"::");
            i += 2;
            continue;
        }

        // Single char special symbols
        if (c == L'@' || c == L'/' || c == L'[' || c == L']' || c == L'(' || c == L')' || c == L',') {
            tokens.push_back(std::wstring(1, c));
            ++i;
            continue;
        }

        // Operators (=, !=, <=, >=, <, >)
        if (isOperatorChar(c)) {
            std::wstring op(1, c);
            if (i + 1 < len && isOperatorChar(input[i + 1])) {
                op.push_back(input[i + 1]);
                ++i;
            }
            tokens.push_back(op);
            ++i;
            continue;
        }

        // Quoted string (single or double quotes)
        if (c == L'"' || c == L'\'') {
            wchar_t quote = c;
            size_t start = i;
            ++i;
            while (i < len && input[i] != quote) {
                // Handle escaped quotes inside string
                if (input[i] == L'\\' && i + 1 < len) {
                    i += 2;
                    continue;
                }
                ++i;
            }
            if (i < len) ++i; // Include closing quote
            tokens.push_back(input.substr(start, i - start));
            continue;
        }

        // Identifiers, numbers, functions
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == '.') {
            size_t start = i;
            while (i < len &&
                (std::isalnum(static_cast<unsigned char>(input[i])) ||
                    input[i] == '_' || input[i] == '-' || input[i] == '.')) {
                ++i;
            }
            tokens.push_back(input.substr(start, i - start));
            continue;
        }

        // Fallback: just push the character
        tokens.push_back(std::wstring(1, c));
        ++i;
    }

    return tokens;
}


// Helper functions for wide strings ----------------------------------
 
 
static bool EqualIgnoreCase(const std::wstring& a, const std::wstring& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (towlower(a[i]) != towlower(b[i])) return false;
    }
    return true;
}

static bool StartsWith(const std::wstring& str, const std::wstring& prefix) {
    if (prefix.length() > str.length()) 
        return false; // Suffix cannot be longer than the main string

    return str.compare(0, prefix.length(), prefix) == 0;
}

static bool EndsWith(const std::wstring& str, const std::wstring& suffix) {
 
    if (suffix.length() > str.length()) 
        return false; // Suffix cannot be longer than the main string

    // Extract a substring from mainString starting at the position where the suffix would begin and compare it with the suffix.
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

static std::wstring Trim(const std::wstring& s) {
    size_t start = 0;
    while (start < s.size() && iswspace(s[start])) start++;
    size_t end = s.size();
    while (end > start && iswspace(s[end - 1])) end--;
    return s.substr(start, end - start);
}

bool AttrContains(const std::map<std::wstring, std::wstring>& attrs, const std::wstring& name, const std::wstring& substring) {
    auto it = attrs.find(name);
    if (it == attrs.end()) return false;
    return it->second.find(substring) != std::wstring::npos;
}

bool AttrStartsWith(const std::map<std::wstring, std::wstring>& attrs, const std::wstring& name, const std::wstring& prefix) {
    auto it = attrs.find(name);
    if (it == attrs.end()) return false;
    return StartsWith(it->second, prefix);
}

bool AttrEndsWith(const std::map<std::wstring, std::wstring>& attrs, const std::wstring& name, const std::wstring& suffix) {
    auto it = attrs.find(name);
    if (it == attrs.end()) return false;
    return EndsWith(it->second, suffix);
}


// Check if any class starts with `prefix`
bool ClassStartsWith(const std::vector<std::wstring>& classlist, const std::wstring& prefix) {
    for (const auto& c : classlist) {
        if (StartsWith(c, prefix))
            return true;
    }
    return false;
}

// Check if any class ends with `suffix`
bool ClassEndsWith(const std::vector<std::wstring>& classlist, const std::wstring& suffix) {
    for (const auto& c : classlist) {
        if (EndsWith(c, suffix))
            return true;
    }
    return false;
}
 
// Check if any class contains `contains`
bool ClassContains(const std::vector<std::wstring>& classlist, const std::wstring& contains) {
    for (const auto& c : classlist) {
        if (c.find(contains) != std::wstring::npos)
            return true;
    }
    return false;
}


std::wstring ClearQuotes(std::wstring val)
{
    if (!val.empty() && val.front() == L'\'') val.erase(val.begin());
    if (!val.empty() && val.back() == L'\'') val.pop_back();
    if (!val.empty() && val.front() == L'"') val.erase(val.begin());
    if (!val.empty() && val.back() == L'"') val.pop_back();
    return val;
}



#endif



