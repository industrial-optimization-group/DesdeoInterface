# Physical interface for desdeo framework

The arduino is loaded with standard firmata: found from arduino ide -> file -> examples -> firmata -> standard firmata

## Todo
* <s>remove stupid files aka pycache</s>
* <s>button refining</s>
    * Okay maybe more button refining, new idea: In button class as a comment
* Test button in interface methods
    * Holding seems fine, might be some delay if also excepting a click or a double click
    * Double click and click don't work well together
        * i.e wait for double clicks and clicks. if double clicks are first then clicks wont register most of the time because reading a double click
* Rotary encoder module
    * Done for now, will continue when i actually receive the component
* Less noise with potentiometers
    * Better pots >:D
* Documentation and commenting standards
* Read on making modules
* (nimbusInterface and others)
    * Nimbus works with one example but not tested with others, so most likely still some issues
* javascript and slider
    * Arduino leonardo incoming!
* How to connect multiple arduinos together and get the input to python from each one nicely