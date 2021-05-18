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
* Semimodular design idea
    * I think the user will always need something to confirmation button and a scroll to select from a list or to choose a value. -So let the master have 2 buttons and a wheel integrated in it for selection, confirmation and declination
        * Other components can be added for different use cases
    * a UI for the physical interface where the user could configure and test the components would be awesome and a looot of work
* Who (which class) takes care that the physical interface gets the correct inputs in the correct time
    * The user could define the problem and then start the interface with the problem and the interface could take care of rest
    * i.e check nimbusinterface or RPMinterface
* Connecting microcontrollers (components) to master and sending the data from slaves to python might be difficult
    * How will each microcontroller know what component it is? Like a button or a rotary encoder?
    * Slave sends to master -> master sends to python