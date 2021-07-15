let connectButton = document.getElementById("connect");
let statusDisplay = document.getElementById("status");
let slider = document.getElementById("slider");
let sliderValue = document.getElementById("potvalue");
let checkBox = document.getElementById("checkbox");
decoder = new TextDecoder();
let device;

sliderValue.innerHTML = slider.value;
slider.onchange = () => sliderValue.innerHTML = slider.value;

async function loopRead() {
  if (!device) {
    console.log('no device');
    return;
  }

  try { //Simplify if possible
    const result = await device.transferIn(5, 64);
    const command = decoder.decode(result.data);
    const val = command.trim();
    if (val.includes("POT")) {
      potValue = parseInt(val.substring(3));
      slider.value = potValue;
      sliderValue.innerHTML = potValue;
    }
    else if (val.includes("BUT")) {
      isDown = val.substring(3) == "DOWN";
      if (isDown) {
        checkBox.checked = true;
      }
      else {
        checkBox.checked = false;
      }
    }

    loopRead()
  } catch (e) {
    console.log('Error reading data', e);
  }
}

async function disconnect() {
  if (!device) {
    return;
  }
  console.log("Closing")
  await device.controlTransferOut({
    requestType: "class",
    recipient: "interface",
    request: 0x22,
    value: 0x00,
    index: 0x02,
  });
  await device.close();
  console.log("Device Closed!")
  device = null;
  connectButton.innerHTML = "Connect";
}

async function connect() {
  try {
    const newDevice = await navigator.usb.requestDevice({
      filters: [{ 'vendorId': 0x2341, 'productId': 0x8036 }],
    });
    device = newDevice;
    await device.open();
    await device.selectConfiguration(1);
    await device.claimInterface(2);
    await device.selectAlternateInterface(2, 0);
    await device.controlTransferOut({
      requestType: "class",
      recipient: "interface",
      request: 0x22,
      value: 0x01,
      index: 0x02,
    });
    console.log("Successfully connected");
    connectButton.innerHTML = "Disconnect";
    loopRead();
  } catch (e) {
    console.log("Failed to Connect: ", e);
  }
}

connectButton.onclick = () => {
  if (device) {
    disconnect();
  } else {
    connect();
  }
};
