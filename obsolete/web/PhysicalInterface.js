let connectButton = document.getElementById("connect");
let startButton = document.getElementById("start");
let nodeContainer = document.getElementById("nodes");

const roles = [
    "obj1",
    "obj2",
    "ojb3",
]

const colors = {
    0: 'green',
    1: 'blue',
    2: 'yellow',
    3: 'orange',
    4: 'cyan',
}

class Node {
    constructor(id, type, pos) {
        this.id = id;
        this.type = type;
        this.pos = pos;
        this.color = colors[type];
        this.element = this.createElement()
    }

    
    createElement() {
        const center = {x: 500, y:500};
        const w = 100;

        var div = document.createElement('div');

        div.setAttribute('id', this.id);
        div.style.position = "absolute";
        div.style.justifyContent = "center";
        div.style.left =  (center.x + (this.pos.x * w))+ "px"
        div.style.top = (center.y + (this.pos.y * w)) + "px"
        div.style.width = w + "px";
        div.style.height = w + "px";
        div.style.backgroundColor = this.color;
        div.style.fontSize = '20px';
        div.innerHTML = this.id === 253 ? "Master" : this.id;

        const dropdown = document.createElement("select")
        dropdown.id = 'roles';
        dropdown.onchange = this.setRole.bind(this);
        roles.forEach(role => {
            const e = document.createElement("option");
            e.value = role;
            e.innerHTML = role;
            dropdown.append(e);
        })
        div.append(dropdown);
        
        return div;
    }

    setRole(role) {
        console.log("TODO: set a role");
    }

    addElement() {
        nodeContainer.append(this.element);
    }

    removeElement() {
        this.element.remove()
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
            
            this.looper = setInterval(this.loop.bind(this), 175);
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
        Object.values(this.nodes).forEach(node => node.removeElement());
        this.device = undefined;
    }

    async loop() {
        if (!this.isConnected()) clearInterval(this.looper);
        try {
            const ser = await this.read();

            if (ser.length > 0){
                // TODO verify crc
                const val = this.parse(ser);
                const command = val['id'];
                switch (command) {
                    case 'N':
                        this.addNode(val['s']);
                        break;
                    case 'O':
                        console.log("O")
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
        console.log(command);
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
        let info = s.split(':');
        const pos = this.calculatePosition(info[2]);
        let node = new Node(Number(info[0]), Number(info[1]), pos)
        this.nodes[Number(info[0])] = node;
        node.addElement();
    }

    calculatePosition(s) {
        let p = {x:0, y:0};
        for (var i = 0; i < s.length; i++){
            if (i == '0') p.y++;
            else if (i == '1') p.x++;
            else if (i == '2') p.y--;
            else if (i == '3') p.x--;
            else if (i == '-') return p;
            else console.log("Unknown direction")
        }
        return p;
    }
}


let intf = new PhysicalInterface();

const connect = () => {
    if (intf.isConnected()) {
        intf.disconnect();
        connectButton.innerHTML = "Connect";
    } else {
        intf.connect();
        intf.quit();
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
