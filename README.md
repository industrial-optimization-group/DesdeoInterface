# Physical interface for desdeo framework

The arduino is loaded with standard firmata: found from arduino ide -> file -> examples -> firmata -> standard firmata

## Todo
* <s>button refining</s>
    * Okay maybe more button refining, new idea: In button class as a comment
* Test button in interface methods
    * Combining to actions together doesn't work well
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
* Better code
    * clean up duplicates
    * readability
    * handlers, encapsulate
    * @property and other decorations


## Next meeting
* Things working:
    * Sliding potentiometer
* Questions:
    * Does someone know how to send and handle signals in python