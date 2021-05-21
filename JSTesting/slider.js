const el_slider = document.getElementById("slider");
const el_value = document.getElementById("value");
const el_slider_range = document.getElementById("range");
const el_button_connect = document.getElementById("connect");
const el_button_print = document.getElementById("print");

let device;
el_button_connect.onclick = () => getDevice();
el_button_print.onclick = () => {
  if (device != undefined) {
    console.log(device.productName)
  } else {console.log("No device connected!")}
};


//C:\Users\Stefu\Downloads\arduino-gh-pages\arduino-gh-pages\library
const slider_min = 0
const slider_max = 1000
el_slider_range.innerHTML = slider_min + ", " + slider_max

el_slider.min = slider_min
el_slider.max = slider_max

el_slider.onchange = () => el_value.innerHTML = slider.value




function getDevice() {
  navigator.usb.requestDevice({ filters: [{ vendorId: 0x2341 }] })
  .then(selectedDevice => {
      device = selectedDevice
    })
    .catch(error => { console.error(error); });
}
