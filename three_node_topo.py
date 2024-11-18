#!/usr/bin/env python

"""Mininet script for testing three-node MIP and routing daemons."""

from mininet.topo import Topo
from mininet.cli import CLI
from mininet.term import tunnelX11
import os
import signal
import time

class ThreeNodeTopo(Topo):
    "Simple topology with three nodes for MIP and routing tests."

    def __init__(self):
        "Set up our custom three-node topology."

        # Initialize topology
        Topo.__init__(self)

        # Add hosts
        A = self.addHost('A')
        B = self.addHost('B')
        C = self.addHost('C')

        # Add links between nodes
        self.addLink(A, B, bw=10, delay='10ms', loss=1.0, use_tbf=False)
        self.addLink(B, C, bw=10, delay='10ms', loss=1.0, use_tbf=False)


terms = []

def openTerm(self, node, title, geometry, cmd="bash"):
    display, tunnel = tunnelX11(node)
    return node.popen(["xterm",
                       "-hold",
                       "-title", "'%s'" % title,
                       "-geometry", geometry,
                       "-display", display,
                       "-e", cmd])

def init_threenode_with_client(self, line):
    "Initialize three nodes with a client on Node C sending to Node A through Node B"
    net = self.mn
    A = net.get('A')
    B = net.get('B')
    C = net.get('C')

    # Start MIP Daemon on Nodes A, B, and C
    terms.append(openTerm(self,
                          node=A,
                          title="MIP A",
                          geometry="80x14+0+0",
                          cmd="./mipd -d usockA 10"))

    terms.append(openTerm(self,
                          node=B,
                          title="MIP B",
                          geometry="80x14+0+450",
                          cmd="./mipd -d usockB 20"))

    terms.append(openTerm(self,
                          node=C,
                          title="MIP C",
                          geometry="80x14+0+900",
                          cmd="./mipd -d usockC 30"))

    # Sleep for 3 sec to ensure that the MIP daemons are ready
    time.sleep(3)

    # Start Routing Daemon on Nodes A, B, and C
    terms.append(openTerm(self,
                          node=A,
                          title="ROUTING A",
                          geometry="80x14+300+0",
                          cmd="./routingd -d usockA"))

    terms.append(openTerm(self,
                          node=B,
                          title="ROUTING B",
                          geometry="80x14+300+450",
                          cmd="./routingd -d usockB"))

    terms.append(openTerm(self,
                          node=C,
                          title="ROUTING C",
                          geometry="80x14+300+900",
                          cmd="./routingd -d usockC"))
    
        # Start a server on Node B
    terms.append(openTerm(self,
                          node=A,
                          title="Server [A]",
                          geometry="80x14+0+900",
                          cmd="./ping_server usockA"))
        # Start a server on Node B
    terms.append(openTerm(self,
                          node=B,
                          title="Server [B]",
                          geometry="80x14+0+900",
                          cmd="./ping_server usockB"))
        # Start a server on Node B
    terms.append(openTerm(self,
                          node=C,
                          title="Server [C]",
                          geometry="80x14+0+900",
                          cmd="./ping_server usockC"))

    # Give time for routing daemons to converge
    time.sleep(30)


    terms.append(openTermWithGDB(self,
                    node=A,
                    title="Client [A]",
                    geometry="80x20+0+300",
                    binary="./ping_client",
                    args="usockA 30 \"Hello from A \""))
    # Start a server on Node B

    terms.append(openTermWithGDB(self,
                        node=B,
                        title="Client [B]",
                        geometry="80x20+0+300",
                        binary="./ping_client",
                        args="usockB 10 \"Hello from B \""))
        # Start a server on Node B
    terms.append(openTermWithGDB(self,
                node=C,
                title="Client [C]",
                geometry="80x20+0+300",
                binary="./ping_client",
                args="usockC 10 \"Hello from C \""))
    time.sleep(3)

def openTermWithGDB(self, node, title, geometry, binary, args):
    display, tunnel = tunnelX11(node)
    gdb_cmd = f"gdb --args {binary} {args}"
    return node.popen(["xterm",
                       "-hold",
                       "-title", "'%s'" % title,
                       "-geometry", geometry,
                       "-display", display,
                       "-e", gdb_cmd])

def init_threenode_with_gdb(self, line):
    "Initialize three nodes and run processes in GDB."
    net = self.mn
    A = net.get('A')
    B = net.get('B')
    C = net.get('C')

    # Start MIP Daemon on Nodes A, B, and C in GDB
    terms.append(openTermWithGDB(self,
                                 node=A,
                                 title="MIP A (GDB)",
                                 geometry="80x14+0+0",
                                 binary="./mipd",
                                 args="-d usockA 10"))

    terms.append(openTermWithGDB(self,
                                 node=B,
                                 title="MIP B (GDB)",
                                 geometry="80x14+0+450",
                                 binary="./mipd",
                                 args="-d usockB 20"))

    terms.append(openTermWithGDB(self,
                                 node=C,
                                 title="MIP C (GDB)",
                                 geometry="80x14+0+900",
                                 binary="./mipd",
                                 args="-d usockC 30"))

    # Sleep for 3 sec to ensure that the MIP daemons are ready
    time.sleep(3)

    # Start Routing Daemon on Nodes A, B, and C in GDB
    terms.append(openTermWithGDB(self,
                                 node=A,
                                 title="ROUTING A (GDB)",
                                 geometry="80x14+300+0",
                                 binary="./routingd",
                                 args="-d usockA"))

    terms.append(openTermWithGDB(self,
                                 node=B,
                                 title="ROUTING B (GDB)",
                                 geometry="80x14+300+450",
                                 binary="./routingd",
                                 args="-d usockB"))

    terms.append(openTermWithGDB(self,
                                 node=C,
                                 title="ROUTING C (GDB)",
                                 geometry="80x14+300+900",
                                 binary="./routingd",
                                 args="-d usockC"))

    # Give time for routing daemons to converge
    time.sleep(30)


    terms.append(openTerm(self,
                          node=A,
                          title="Server [A]",
                          geometry="80x14+0+900",
                          binary="./ping_server",
                          cmd="usockA"))

    terms.append(openTerm(self,
                          node=A,
                          title="Server [B]",
                          geometry="80x14+0+900",
                          binary="./ping_server",
                          cmd="usockB"))

            # Start a server on Node B
    terms.append(openTerm(self,
                          node=A,
                          title="Server [C]",
                          geometry="80x14+0+900",
                          binary="./ping_server",
                          cmd="usockC"))
    
    terms.append(openTermWithGDB(self,
                        node=A,
                        title="Client [A]",
                        geometry="80x20+0+300",
                        binary="./ping_client",
                        args="usockA 10 \"Hello from A \""))
        # Start a server on Node B

    terms.append(openTermWithGDB(self,
                        node=B,
                        title="Client [B]",
                        geometry="80x20+0+300",
                        binary="./ping_client",
                        args="usockB 20 \"Hello from B \""))
        # Start a server on Node B
    terms.append(openTermWithGDB(self,
                node=C,
                title="Client [C]",
                geometry="80x20+0+300",
                binary="./ping_client",
                args="usockC 30 \"Hello from C \""))


# Mininet Callbacks
CLI.do_init_threenode_with_gdb = init_threenode_with_gdb


# Mininet Callbacks

# Inside mininet console run `init_threenode_with_client`
CLI.do_init_threenode_with_client = init_threenode_with_client

# Inside mininet console run 'EOF' to gracefully kill the mininet console
orig_EOF = CLI.do_EOF

def do_EOF(self, line):
    for t in terms:
        os.kill(t.pid, signal.SIGKILL)
    return orig_EOF(self, line)

CLI.do_EOF = do_EOF

# Topologies
topos = {
    'threenode': (lambda: ThreeNodeTopo())
}

# Instructions for running this script:
# 1. Ensure that the script is executable:
#    chmod +x three_node_mip_script.py
#
# 2. Start Mininet with the custom topology defined here:
#    sudo -E mn --mac --custom three_node_topo.py --topo threenode --link tc
#
# 3. Once inside the Mininet CLI, run the following command to initialize the nodes:
#    init_threenode_with_client
#
# 4. This will:
#    - Start the MIP daemons on nodes A, B, and C.
#    - Start the ROUTING daemons on nodes A, B, and C.
#    - Launch a client on Node C to send a message to Node A via Node B.
#    - Launch a server on Node A to receive the message.
#
# 5. To exit gracefully, type `EOF` in the Mininet CLI:
#    mininet> EOF