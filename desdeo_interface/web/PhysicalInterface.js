let connectButton = document.getElementById("connect");
let startButton = document.getElementById("start");

class Node {
    constructor(id, type, pos) {
        this.id = id;
        this.type = type;
        this.pos = pos;
    }
}

class PhysicalInterface {
    constructor() {
        this.decoder = new TextDecoder();
        this.encoder = new TextEncoder();
        this.device = undefined;
        this.started = false;
        this.nodes = {};
    }

    async connect() {
        if (this.isConnected()) return;
        try {
            const newDevice = await navigator.usb.requestDevice({
                filters: [{ 'vendorId': 0x2341, 'productId': 0x8036 }],
            });
            this.device = newDevice;
            await this.device.open();
            await this.device.selectConfiguration(1);
            await this.device.claimInterface(2);
            await this.device.selectAlternateInterface(2, 0);
            await this.device.controlTransferOut({
                requestType: "class",
                recipient: "interface",
                request: 0x22,
                value: 0x01,
                index: 0x02,
            });
            console.log("Successfully connected");
            setInterval(this.loop.bind(this), 500);
        } catch (e) {
            console.log("Failed to Connect: ", e);
        }
    }

    async disconnect() {
        if (!this.isConnected()) return;
        console.log("Closing")
        await this.device.controlTransferOut({
            requestType: "class",
            recipient: "interface",
            request: 0x22,
            value: 0x00,
            index: 0x02,
        });
        await this.device.close();
        console.log("Device Closed!")
        this.device = undefined;
    }

    async loop() {
        try {
            const ser = await this.read();
            console.log(ser);

            if (ser.length > 0){
                // TODO verify crc
                const val = this.parse(ser);
                const command = val['id'];
                switch (command) {
                    case 'N':
                        this.addNode(val['s']);
                        break;
                    case 'O':
                        this.handleConfiguration();
                        break;
                    case 'V':
                        console.log(val['s']);
                        break;
                    default:
                        console.log(ser);
                        break;
                }
            }
        } catch (e) {
            console.log('Error reading data', e);
        }
    }

    async start() {
        if (!this.isConnected()) return;
        console.log("sending 'S'")
        this.send("S");
        this.started = true;
    }

    async quit() {
        if (!this.isConnected()) return;
        console.log("sending 'Q'")
        this.send("Q");
        this.started = false;
    }

    async read() {
        if (!this.isConnected()) return;
        const result = await this.device.transferIn(5, 64);
        const command = this.decoder.decode(result.data);
        const val = command.trim();
        return val;
    }

    async send(s) {
        const es = this.encoder.encode(s);
        this.device.transferOut(4, es).catch(
            error => {
                console.log(error);
            }
        );
    }

    isConnected() {
       return (this.device != undefined)
    }

    parse(s) {
        const arr = s.split(' ');
        const filt = arr.filter(x => {return x !== ""})
        const n = filt.length - 1;

        const values = {
            'id': filt[0],
            's': filt[1],
            'crc': filt[n]
        }
        return values;
    }

    addNode(s) {
        console.log(s)
        let info = s.split(':').map(x => {return Number(x)});
        const pos = this.calculatePositions(info[2]);
        let node = new Node(info[0], info[1], pos)
        this.nodes[info[0]] = node;
    }

    calculatePositions(s) {
        console.log("Calculate positions");
        return s;
    }

    handleConfiguration() {
        console.log("handle configuration");
        return;
    }
}


let intf = new PhysicalInterface();

const connect = () => {
    if (intf.isConnected()) {
        intf.disconnect();
        connectButton.innerHTML = "Connect";
    } else {
        intf.connect();
        connectButton.innerHTML = "Disconnect";
    }
}

const start = () => {
    if (!intf.isConnected()) return;

    if (intf.started) {
        intf.quit();
        startButton.innerHTML = "Start";
    }
    else {
        intf.start();
        startButton.innerHTML = "Quit";
    }
        
}
