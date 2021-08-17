### Setup
This module is intended to be used with the [web interface](https://github.com/gialmisi/desdeo-frontend) of DESDEO framework. Currently this module works only with the reference point method.

You could make this an own package but for now/simplicity:
* copy physical_interface folder to react projects src folder, or whatever folder you're using
* import Interface to ReferencePointMethod.tsx 
* Create the PhysicalInterface component.
    * I'd suggest adding it to the end of the page
    * And only rendering it if showFinal is false and fetchedInfo is true: 
    ```
     {!showFinal && fetchedInfo && <PhysicalInterface problem={activeProblemInfo}></PhysicalInterface>}
     ```
* The module assumes that important action buttons such as set/iterate and input fields have an id
    * Objective input field ids should match the objective names of the activeProblemInfo.
    * buttons/toggles ids can be adjusted but make sure they match the ids in the Interface.js file
        * ```
        let iterateB = document.getElementById("iterate");
        let setB = document.getElementById("set");
        let satisfiedSwitch = document.getElementById("satisfied-switch");
        let stopB = document.getElementById("stop");
        ```



### How to use

After the setup a physical interface component should appear in the rpm view. Clicking the connect button will open a dialog which lists available/supported boards connected via usb, (the code only looks for leonardos/promicros). To connect a device just click the desired device from the dialog. After this the connect button should be replaced with a disconnect button and a new button 'start' should appear next to it. 

By clicking the start button a 'S' or start command command is sent to the device which triggers the configuration process. A figuring the layout loader should appear and after a very short while the nodes should pop below the buttons. The nodes should be arranged the same way as they are in the physical setup assuming the usb port is on top.

Each node should also display it's type, components, component values and a dialog for each component. With the dialog the dm can choose a role for the component. i.e. for a rotary encoder component the role could be a certain objective function or to scroll solutions. For a button it could be iterating or stopping the iteration process and so on.

When assign a objective function role to a component a bounds packet is send to the device which will bound the component values between nadir and ideal of that objective.

So to use the physical interface assign any roles you wish to any components you wish and start solving.

If a node is removed from the physical interface after the configuration the corresponding node in the page should turn red (Currently only that node even if some other nodes would be disconnected with that one, software TODO). 

If a node is connected to the physical interface after the configuration a new button 'reconfigure' should appear below the two buttons. Clicking this button will run the configuration again and that node should be found. (TODO don't reset the roles...)

If two or more non-button components have the same role the value will be set by the latest updated value:
say r1 and r2 are responsible of obj1. The dm adjust the obj1 value to 0.5 with r1 and then to 1.7 with r2. If the dm now adjusts obj1 value with r1 the values will start from where that component had last left so 0.5. This can be considered as a feature ;)