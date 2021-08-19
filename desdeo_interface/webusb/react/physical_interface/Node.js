/*
Handles nodes  
*/


import React, { useEffect, useState } from 'react';
import Comp from './Component';
const types = {
    0: ['EMPTY', [], "white"],
    1: ['BUTTON',["/button.png"], "slategrey"],
    2: ['POTENTIOMETER', ["/fader.png"], "slategrey"],
    3: ['ROTARY ENCODER', ["/rotary.png"], "slategrey"],
    4: ['POTENTIOMETER + \n BUTTON', ["/fader.png", "/button.png"], "slategrey"],
    5: ['ROTARY ENCODER + \n BUTTON', ["/rotary.png", "/button.png"], "slategrey"],
    255: ['EMPTY', [], "white"],
    111: ['DISCONNECTED', [], "red"],
}

function Node(props) {
    const size = props.nodeSize
    const type = types[props.type][0];
    const images = types[props.type][1];
    const color = types[props.type][2];
    const id = props.id
    const nodeX = size;
    const nodeY = ((props.node.isLong) ? size + size / 2 : size);
    let comps = props.comps

    let bottom = props.pos.y * nodeX - (props.node.isLong ?  size / 2  : 0)

    console.log(comps)

    return (
            <div id={id} style={{
                position:"absolute",
                border: "2px solid black",
                left: (props.pos.x * nodeX) + "px",
                bottom:  bottom + "px",
                width: nodeX + "px",
                height: nodeY + "px",
                backgroundColor: color,
            }}>
                <p style={{fontSize:18}}>{(id === 0 ? "Master" : "Node " + id) + " : " + type}</p>
                {images.length > 0 && images.map(image => (
                    <img 
                        src={image}
                        style = {{position:'relative'}}
                    />))
                }

                <div>
                    {Object.keys(comps).map(ctype => (
                        Object.keys(comps[ctype]).map(cid => (
                            <Comp 
                                value={comps[ctype][cid]["value"]}
                                type={ctype}
                                key={ctype + cid}
                                cid= {cid}
                                objectiveNames={props.objectiveNames}
                                nodeId = {id}
                                setNodes={props.setNodes}
                                setBounds={props.setBounds}
                            />
                        ))
                    ))}
                </div>
            </div>
    )
}

export default Node