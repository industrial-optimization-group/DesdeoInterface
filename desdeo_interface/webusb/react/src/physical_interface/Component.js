/*
Handles components and setting component roles
*/

import React, { useEffect, useState } from 'react';
import { Form } from 'react-bootstrap';

function Comp(props) {
    const type = props.type;
    const value = props.value;
    const id = props.nodeId;
    const cid = props.cid;

    console.log(type + cid)

    let roles = type === 'B' ? [
        "Assign role",
        "iterate",
        "confirm"
    ] : ["Assign role", ...props.objectiveNames, "scroll solutions"];

    /**
   * Sets a component role
   */
    const setRole = (event) => {
        if (event.target === undefined) return;
        props.setNodes(prev => {
            if (prev[id] !== undefined)
                prev[id]["components"][type][cid]["role"] = event.target.value;
            return [...prev];
        })
        props.setBounds(id, type, cid, event.target.value)
    }

    return (
            <Form>
                <Form.Text style={{fontSize:18}}> {"Component: " + type + cid + ":\tValue: " + value}</Form.Text>
                <Form.Control
                        as="select"
                        onChange={setRole}
                        size="sm"
                    >
                        {roles.map(role => {
                            return <option value={role}>
                                {role}
                            </option>
                        })}
                    </Form.Control>
            </Form>             
    );
}

export default Comp