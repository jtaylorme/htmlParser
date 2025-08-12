# htmlparser

A simple c++ html parser with limited xPath support.  

See Wiki for documentation.

All my work has been on Windows and no good way to test other platforms so may need a bit of tweaking if you port.



 
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

Basic usage please see demo [parser_test.cpp](parser_test.cpp).

