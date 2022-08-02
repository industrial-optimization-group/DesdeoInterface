/*
Main component of the physical interface. Handles communication, setting bounds/values, displaying the physical interface. 
*/

import Node from "./Node";
import React, { useState, useEffect, useRef } from "react";
import ReactLoading from "react-loading";
import { Button } from "react-bootstrap";
import Device from "./Device";

// probleminfo : activeprobleminfo
function PhysicalInterface(probleminfo) {
  let nodeSize = 60 * 5.5;

  // gets the data from the device every interval. setinterval
  let looper = undefined;

  // Html elements from the method view. A physical button press can be easily be bound to simulate a button element click.
  // This should not be the final way of accomplishing this.
  let iterateB = document.getElementById("iterate");
  let setB = document.getElementById("set");
  let satisfiedSwitch = document.getElementById("satisfied-switch");
  let stopB = document.getElementById("stop");

  const [inputs, setInputs] = useState({}); // Objective input fields. I've set each input an id that matches the objective name.
  const [device, setDevice] = useState(new Device()); // The device connected to usb
  const [connected, setConnected] = useState(false); // Is the device connected
  const [started, setStarted] = useState(false); // Is the physical interface started. Connected != started. but connecting could initialize start.
  const [configured, setConfigured] = useState(false); // Is the physical interface configured : Nodes found
  const [nodes, setNodes] = useState([]); // All the nodes in the configuration. List of object, initially was just an object which was nicer but some react state issues which i didn't feel like handling
  const [newConnection, setNewConnection] = useState(false); // Has a new node connected
  const [data, setData] = useState(""); // Latest data from the device/serial

  /**
   *  Set a nodes component bounds
   * @param {nid} node id
   * @param {ctype} component type
   * @param {cid} component id
   * @param {role} component role
   */
  const setBounds = (nid, ctype, cid, role) => {
    if (role === "scroll solutions") {
      sendBounds(nid, ctype, cid, 0, 4, 1); // hard coded as it seems like solution count is set to 5
      return;
    }
    let problem = probleminfo.problem;
    const i = probleminfo.problem.objectiveNames.findIndex((el) => role === el);
    if (i < 0) return;
    problem.minimize[i] === 1
      ? sendBounds(nid, ctype, cid, problem.ideal[i], problem.nadir[i])
      : sendBounds(nid, ctype, cid, problem.nadir[i], problem.ideal[i]);
  };

  // Get the input fields when component has mounted
  useEffect(() => {
    let inps = {};
    probleminfo.problem.objectiveNames.map(
      (objName) => (inps[objName] = document.getElementById(objName))
    );
    console.log(inps);
    setInputs(inps);
  }, []);

  // Handle new data when data state changes
  useEffect(() => {
    handleData();
  }, [data]);

  const connect = async () => {
    let success = await device.connect();
    setConnected(success);
  };

  const disconnect = async () => {
    device.disconnect();
    setConnected(false);
    resetState();
  };

  /**
   * Updates the data state by reading the device/serial
   */
  const getData = async () => {
    if (!connected) {
      return;
    }
    try {
      const ser = await device.read();
      console.log(ser);
      if (ser !== undefined) {
        console.log("setting data");
        setData(ser);
      }
    } catch (e) {
      console.log("error reading data", e);
      disconnect();
    }
  };

  /**
   * Reset the states and clear the looper
   */
  const resetState = () => {
    setNodes([]);
    setStarted(false);
    setConfigured(false);
    if (looper !== undefined) {
      clearInterval(looper);
      looper = undefined;
    }
  };

  /**
   * Handles new data
   */
  const handleData = () => {
    if (!connected) return;
    let d = data;
    console.log("Data: ", d);
    if (d === "") return;
    console.log("Data: ", d);
    const val = parse(d);
    if (val === undefined) return;
    const command = val["id"];
    switch (command) {
      case "N":
        addNode(val["s"]);
        break;
      case "O":
        console.log("O");
        setConfigured(true);
        break;
      case "V":
        handleV(val["s"]);
        break;
      case "D":
        handleDisconnection(val["s"]);
        break;
      case "C":
        handleConnection();
        break;
      default:
        console.log(data);
        break;
    }
  };

  /**
   * Start the configuration and the looper
   */
  const start = () => {
    if (!connected) return;
    quit(); // just in case
    setStarted(true);
    looper = setInterval(getData, 30);
    device.send("S");
  };

  /**
   * Stop using the physical interface. Device is still connected
   */
  const quit = () => {
    if (!connected) return;
    console.log("sending 'Q'");
    device.send("Q");
    resetState();
  };

  /**
   * Start the configuration and the looper
   * @param {s} datastring
   */
  const parse = (s) => {
    if (s === undefined) return s;
    const arr = s.split(" ");
    const filt = arr.filter((x) => {
      return x !== "";
    });
    const n = filt.length - 1;

    const values = {
      id: filt[0],
      s: filt[1],
      crc: filt[n],
    };
    return values;
  };

  /**
   * Get the components of a node with the type
   * Could be improved a lot
   * @param {type} Nodes type
   */
  const getComps = (type) => {
    if (type === 0) return [];
    else if (type === 1) return { B: { 0: { value: 0, role: "undefined" } } };
    else if (type === 2) return { P: { 0: { value: 0, role: "undefined" } } };
    else if (type === 3)
      return {
        B: { 0: { value: 0, role: "undefined" } },
        R: { 0: { value: 0, role: "undefined" } },
      };
    else if (type === 4)
      return {
        B: { 0: { value: 0, role: "undefined" } },
        P: { 0: { value: 0, role: "undefined" } },
      };
    else if (type === 5)
      return {
        B: {
          0: { value: 0, role: "undefined" },
          1: { value: 0, role: "undefined" },
        },
        R: { 0: { value: 0, role: "undefined" } },
      };
  };

  /**
   * Handles a value packet
   * @param {s} datastring of type nodeid:componenttype:componentid:componentvalue
   */
  const handleV = (s) => {
    let [nid, ct, cid, val] = s.split(":");
    val = Number(val);
    cid = Number(cid);
    nid = Number(nid);
    if (nid === 254) nid = 0;

    setNodes((prev) => {
      if (prev[nid] !== undefined) prev[nid].components[ct][cid]["value"] = val;
      return [...prev];
    });

    handleUpdatedValues(nid, ct, cid, val);
  };

  /**
   * Handles a disconnection packet
   * @param {s} datastring
   */
  const handleDisconnection = (s) => {
    let [nid, d] = s.split(":");
    if (nodes[nid] === undefined) return;
    nid = Number(nid);
    let dir = convertToDir(d);
    let pos = { x: nodes[nid].pos.x + dir.x, y: nodes[nid].pos.y + dir.y };
    console.log(pos);
    let disconnectedNode = findNodeByPos(pos);
    if (disconnectedNode !== undefined) {
      console.log("Node " + disconnectedNode.id + " disconnected!");
      var nods = [...nodes];
      nods[disconnectedNode.id].type = 111; // Master cannot disconnect and otherwise id == index
      setNodes(nods);
    }
  };

  /**
   * Handles a connection packet
   */
  const handleConnection = () => {
    console.log("C");
    setNewConnection(true);
  };

  /**
   * Sends bounds packet to the device
   * @param {nid} node id
   * @param {ctype} component type
   * @param {cid} component id
   * @param {min} minimum bound
   * @param {max} maximum bound
   * @param {step} step size for rotary encoders
   */
  const sendBounds = (nid, ctype, cid, min, max, step = 0) => {
    step = step === 0 ? Math.abs((max - min) / 75) : (step = step);
    const s = `B ${nid}:${ctype}:${cid}:${min}:${max}:${step}`;
    console.log(s);
    device.send(s);
  };

  /**
   * Handles a new node by adding it to the nodes state
   * @param {s} datastring
   */
  const addNode = (s) => {
    let [id, type, p] = s.split(":");
    id = Number(id);
    if (id === 254) id = 0;
    type = Number(type);
    const pos = calculatePosition(p);
    let comps = getComps(type);
    let node = {
      id: id,
      type: type,
      pos: pos,
      posString: p,
      components: comps,
      role: "undefined",
      isLong: isLong(type),
    };
    setNodes((prev) => {
      return [...prev, node];
    });
    console.log(nodes);
  };

  /**
   * Converts a direction number to a position object
   * @param {dir} direction, should be between 0-3
   */
  const convertToDir = (dir) => {
    if (dir == "0") return { x: 0, y: 1 };
    else if (dir == "1") return { x: 1, y: 0 };
    else if (dir == "2") return { x: 0, y: -1 };
    else if (dir == "3") return { x: -1, y: 0 };
    console.log("got invalid direction");
    return undefined;
  };

  /**
   * Calculates a nodes position based on the position string
   * @param {s} datastring
   */
  const calculatePosition = (s) => {
    if (s[0] == "-") return { x: 0, y: 0 }; // Master

    let node = findNodeByPosString(s.slice(0, -1)); // Node that found this node
    let p = Object.assign({}, node.pos); // Copy node pos object

    let dir = convertToDir(s.slice(-1)); // Get direction of last movement
    console.log(dir);
    p.y += dir.y * (node.isLong && dir.y == -1 ? 1.5 : 1);
    p.x += dir.x;

    return p;
  };

  /**
   * Determines whether a node is long by its type
   * currently only potentiometer nodes are long
   * @param {type} node type
   */
  const isLong = (type) => {
    return type === 2 || type === 4;
  };

  /**
   * Find a node from nodes based on its position
   * @param {pos} nodes position
   */
  const findNodeByPos = (pos) => {
    if (nodes === undefined) return;
    for (var i = 0; i < nodes.length; i++) {
      let npos = nodes[i].pos;
      if (npos.x === pos.x && npos.y === pos.y) return nodes[i];
    }
    return undefined;
  };

  /**
   * Find a node from nodes based on its position string
   * @param {ps} nodes position string
   */
  const findNodeByPosString = (ps) => {
    if (nodes === undefined) return;
    if (ps == "") return nodes[0]; // master
    for (var i = 1; i < nodes.length; i++)
      if (ps === nodes[i].posString) return nodes[i];
    return undefined;
  };

  /**
   * Re run the configuration. i.e if a new node connects and dm decides so
   * @param {s} event
   */
  const reConfigure = (s) => {
    setNewConnection(false);
    start();
  };

  /**
   * Sets the updated component values to actual elements
   * @param {id} node id
   * @param {ct} component type
   * @param {cid} component id
   * @param {val} component value
   */
  const handleUpdatedValues = (id, ct, cid, val) => {
    let node = nodes[id];
    if (node === undefined) return;
    let role = node["components"][ct][cid]["role"];

    if (role in inputs) {
      inputs[role].value = val;
      setB.click();
    } else if (role === "iterate") {
      if (val === 1) iterateB.click();
    } else if (role === "confirm") {
      if (val === 1) {
        if (satisfiedSwitch === null || satisfiedSwitch === undefined) return;
        satisfiedSwitch.click();
        setTimeout(() => {
          stopB = document.getElementById("stop");
          if (stopB === null || stopB === undefined) return;
          stopB.click();
        }, 1000);
      }
    } else if (role === "scroll solutions") {
      const tabs = document.getElementsByClassName("list-group-item-action");
      if (tabs.length > 0) {
        if (tabs[val] === null || tabs[val] === undefined) return;
        tabs[val].click();
        console.log("scroll");
      }
    }
  };

  /**
   * Get the width and height of the node container
   */
  const getRange = () => {
    let maxmin = { xmin: 0, ymin: 0, xmax: 0, ymax: 0 };
    for (let i = 0; i < nodes.length; ++i) {
      let npos = nodes[i].pos;
      if (npos.x < maxmin.xmin) maxmin.xmin = npos.x;
      else if (npos.x > maxmin.xmax) maxmin.xmax = npos.x;
      if (npos.y < maxmin.ymin) maxmin.ymin = npos.y;
      else if (npos.y > maxmin.ymax) maxmin.ymax = npos.y;
      if (nodes[i].isLong) maxmin.ymin -= 0.5;
    }
    return {
      x: maxmin.xmax - maxmin.xmin + 1, // +1 because master
      y: maxmin.ymax - maxmin.ymin + 1, // +1 because master
      ymin: maxmin.ymin,
      xmin: maxmin.xmin,
    };
  };

  let range = getRange();
  let offset = { x: -range.xmin, y: -range.ymin }; // Nodes are set with absolute positions from bottomleft, master should be centered.

  return (
    <div>
      <h3>Physical interface</h3>
      <p>All changes are automatically saved</p>
      <Button onClick={connected ? disconnect : connect}>
        {" "}
        {connected ? "Disconnect" : "Connect"}{" "}
      </Button>
      {connected && (
        <Button onClick={started ? quit : start}>
          {started ? "Quit" : "Start"}
        </Button>
      )}
      {connected && newConnection && (
        <div>
          <p>A new node detected</p>
          <Button onClick={reConfigure}>Reconfigure</Button>
        </div>
      )}
      <div // Match the size of the nodes
        style={{
          //border: "2px black solid",
          margin: "25px auto 25px",
          width: range.x * nodeSize + "px",
          height: range.y * nodeSize + "px",
          position: "relative",
        }}
      >
        {started &&
          (!configured ? (
            <div>
              {"Figuring the layout... "}
              <ReactLoading
                type={"bubbles"}
                color={"#ffffff"}
                className={"loading-icon"}
                height={28}
                width={32}
              />
            </div>
          ) : (
            nodes.map((node, id) => (
              <Node
                id={node.id}
                key={id}
                node={node}
                type={node.type}
                pos={{ x: node.pos.x + offset.x, y: node.pos.y + offset.y }}
                nodeSize={nodeSize}
                comps={node.components}
                objectiveNames={probleminfo.problem.objectiveNames}
                setNodes={setNodes}
                setBounds={setBounds}
              />
            ))
          ))}
      </div>
      <br />
      <br />
      <br />
    </div>
  );
}

export { PhysicalInterface };
