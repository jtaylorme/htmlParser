# htmlparser

a simple c++ html parser

## Intro

- support html and xhtml document
- support getElementById(ClassName/TagName)
- support simple XPath select interface

## Changes from rangerlee version
-TagName Comparison CaseInsensitive
-Changed html method to OuterHTML

-Added InnerHTML
-Added SetAttribute
-Added SetInnerHTML
-Added SetInnerText
-Added GetElementsById
-Moved to classlist style handling of classes
    std::vector<std::string> classlist;    
-Added GetClassList
-Added HasClass
-Added RemoveClass
-Added ToggleClass
-Added ClearClasses
-Added GetSiblingNext
-Added GetSiblingPrev
-Added GetChildren
-Added Removed SplitClassName
-Added GetRoot

-Added Helper Functions
  UpdateClassAttribute
  toLower
  toLowerW
  EscapeForXPath

## Usage

Basic usage please see demo [test.cpp](test.cpp).

