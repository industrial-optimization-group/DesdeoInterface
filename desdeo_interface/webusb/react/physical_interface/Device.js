/*
Webusb device  
*/

class Device {
  constructor() {
    console.log("Device");
    this.decoder = new TextDecoder();
    this.encoder = new TextEncoder();
    this.device = undefined;
    this.connected = false;
  }


    /**
   * Connect to a webusb supported device
   */
  connect = async () => {
    if (this.connected) {
      console.log("Already connected");
      return false;
    }
    try {
      const newDevice = await navigator.usb.requestDevice({
        filters: [{ vendorId: 0x2341, productId: 0x8036 }], // check for pro micro/leonardo
      });
      await newDevice.open();
      await newDevice.selectConfiguration(1);
      await newDevice.claimInterface(2);
      await newDevice.selectAlternateInterface(2, 0);
      await newDevice.controlTransferOut({
        requestType: "class",
        recipient: "interface",
        request: 0x22,
        value: 0x01,
        index: 0x02,
      });
      this.device = newDevice;
      console.log("Successfully connected");
      this.connected = true;
      return true;
    } catch (e) {
      console.log("Failed to Connect: ", e);
      return false;
    }
  };


   /**
   * Disconnects the webusb device
   */
  disconnect = async () => {
    if (!this.connected) {
        console.log("Device not even connected");
        return;
    }
    console.log("disconnect")
    try {
        await this.device.controlTransferOut({
            requestType: "class",
            recipient: "interface",
            request: 0x22,
            value: 0x00,
            index: 0x02,
          });
          await this.device.close();
          console.log("Device Closed!");
          this.connected = false;
          this.device = undefined;
    } catch (e) {
        this.device = undefined;
        this.connected = false;
    }
  };

   /**
   * Read the webusb serial
   */
  read = async () => {
    if (!this.connected) return;
    const result = await this.device.transferIn(5, 64);
    const command = this.decoder.decode(result.data);
    const val = command.trim();
    console.log(val);
    return val;
  };

    /**
   * Send a string through the webusb serial
   */
  send = (s) => {
    if (!this.connected || this.device === undefined) return;
    console.log("sending " + s);
    const es = this.encoder.encode(s);
    this.device.transferOut(4, es).catch((error) => {
      console.log(error);
    });
  };
}

export default Device;
