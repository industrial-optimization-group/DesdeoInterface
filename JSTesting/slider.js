

const el_slider = document.getElementById("slider")
const el_value = document.getElementById("value") 
const el_slider_range = document.getElementById("range") 

const slider_min = 0
const slider_max = 1000
el_slider_range.innerHTML = slider_min + ", " + slider_max

el_slider.min = slider_min
el_slider.max = slider_max

el_slider.onchange = () => el_value.innerHTML = slider.value


navigator.usb.requestDevice({ filters: [{ vendorId: 0x2341 }] })
.then(device => {
  console.log(device.productName);      // "Arduino Micro"
  console.log(device.manufacturerName); // "Arduino LLC"
})
.catch(error => { console.error(error); });