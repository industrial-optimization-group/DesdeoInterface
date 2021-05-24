(function() {
  'use strict';

  document.addEventListener('DOMContentLoaded', event => {
    const slider = document.getElementById("slider");
    const valueText = document.getElementById("value");
    const sliderRange = document.getElementById("range");
    const connectButton = document.getElementById("connect");
    const printButton = document.getElementById("print");

    let device;
    connectButton.onclick = () => {
      if (device == undefined) {connect();}
      else {disconnect();}
    };

    printButton.onclick = () => {
      if (device != undefined) {
        console.log(device.productName);
      } else {console.log("No device connected!")}
    };

    const sliderMin = 0
    const sliderMax = 1000
    sliderRange.innerHTML = sliderMin + ", " + sliderMax

    slider.min = sliderMin
    slider.max = sliderMax

    slider.onchange = () => valueText.innerHTML = slider.value


    // function connect() {
    //   navigator.usb.requestDevice({ filters: [{ vendorId: 0x2341 }] })
    //   .then(selectedDevice => {
    //       device = selectedDevice;
    //       connectButton.innerHTML = "Disconnect";
    //       return device.open();
    //     })
    //     .catch(error => { console.error(error); });
    // }


    function disconnect() {
      connectButton.innerHTML = "Connect";
      device.close()
      device = undefined
    }

    function onUpdate() {
      if (device == undefined) {return;}
    }


    let port;

    function connect() {
      port.connect().then(() => {
        statusDisplay.textContent = '';
        connectButton.textContent = 'Disconnect';

        port.onReceive = data => {
          let textDecoder = new TextDecoder();
          console.log(textDecoder.decode(data));
        }
        port.onReceiveError = error => {
          console.error(error);
        };
      }, error => {
        statusDisplay.textContent = error;
      });
    }
  });
})();