#!/usr/bin/env python

"""Mininet script for testing two-node MIP and routing daemons, with separate options for running with or without gdb."""

from mininet.topo import Topo
from mininet.cli import CLI
from mininet.term import tunnelX11
import os
import signal
import time

# Usage example:
# sudo -E mn --mac --custom two_node_mip_routing_script.py --topo twonode --link tc

class TwoNodeTopo(Topo):
    "Simple topology with two nodes for MIP and routing tests."

    def __init__(self):
        "Set up our custom two-node topology."

        # Initialize topology
        Topo.__init__(self)

        # Add hosts
        A = self.addHost('A')
        B = self.addHost('B')

        # Add link between A and B with 1% packet loss and 10ms delay
        self.addLink(A, B, bw=10, delay='10ms', loss=1.0, use_tbf=False)


terms = []

def openTerm(self, node, title, geometry, cmd="bash"):
    display, tunnel = tunnelX11(node)
    return node.popen(["xterm",
                       "-hold",
                       "-title", "'%s'" % title,
                       "-geometry", geometry,
                       "-display", display,
                       "-e", cmd])


def init_twonode_with_gdb(self, line):
    "Initialize both nodes with gdb debugging"
    net = self.mn
    A = net.get('A')
    B = net.get('B')

    # Start MIP Daemon on Node A and Node B with gdb
    terms.append(openTerm(self,
                          node=A,
                          title="MIP A (gdb)",
                          geometry="80x14+0+0",
                          cmd="gdb --args ./mipd -d usockA 10"))

    terms.append(openTerm(self,
                          node=B,
                          title="MIP B (gdb)",
                          geometry="80x14+0+450",
                          cmd="gdb --args ./mipd -d usockB 20"))

    # Sleep for 3 sec to ensure that the MIP daemons are ready
    time.sleep(3)

    # Start Routing Daemon on Node A and Node B with gdb
    terms.append(openTerm(self,
                          node=A,
                          title="ROUTING A (gdb)",
                          geometry="80x14+0+210",
                          cmd="gdb --args ./routingd -d usockA"))

    terms.append(openTerm(self,
                          node=B,
                          title="ROUTING B (gdb)",
                          geometry="80x14+0+660",
                          cmd="gdb --args ./routingd -d usockB"))

    # Give time for routing daemons to converge
    time.sleep(15)


def init_twonode_without_gdb(self, line):
    "Initialize both nodes without gdb"
    net = self.mn
    A = net.get('A')
    B = net.get('B')

    # Start MIP Daemon on Node A and Node B without gdb
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

    # Sleep for 3 sec to ensure that the MIP daemons are ready
    time.sleep(3)

    # Start Routing Daemon on Node A and Node B without gdb
    terms.append(openTerm(self,
                          node=A,
                          title="ROUTING A",
                          geometry="80x14+0+210",
                          cmd="./routingd -d usockA"))

    terms.append(openTerm(self,
                          node=B,
                          title="ROUTING B",
                          geometry="80x14+0+660",
                          cmd="./routingd -d usockB"))

    # Give time for routing daemons to converge
    time.sleep(15)

def init_twonode_with_client_and_gdb(self, line):
    "Initialize both nodes with gdb and start client on A"
    net = self.mn
    A = net.get('A')
    B = net.get('B')

    # Start MIP Daemon on Node A and Node B with gdb
    terms.append(openTerm(self,
                          node=A,
                          title="MIP A (gdb)",
                          geometry="80x14+0+0",
                          cmd="gdb --args ./mipd -d usockA 10"))

    terms.append(openTerm(self,
                          node=B,
                          title="MIP B (gdb)",
                          geometry="80x14+0+450",
                          cmd="gdb --args ./mipd -d usockB 20"))

    # Sleep for 3 sec to ensure that the MIP daemons are ready
    time.sleep(3)

    # Start Routing Daemon on Node A and Node B with gdb
    terms.append(openTerm(self,
                          node=A,
                          title="ROUTING A (gdb)",
                          geometry="80x14+0+210",
                          cmd="gdb --args ./routingd -d usockA"))

    terms.append(openTerm(self,
                          node=B,
                          title="ROUTING B (gdb)",
                          geometry="80x14+0+660",
                          cmd="gdb --args ./routingd -d usockB"))

    # Give time for routing daemons to converge
    time.sleep(15)

    # Start client on Node A with gdb
    terms.append(openTerm(self,
                          node=A,
                          title="Client [A] (gdb)",
                          geometry="80x14+0+900",
                          cmd="gdb --args ./ping_client usockA 20 \"Hello from A\""))




def init_twonode_with_client(self, line):
    "Initialize both nodes without gdb and start client on A"
    net = self.mn
    A = net.get('A')
    B = net.get('B')

    # Start MIP Daemon on Node A and Node B without gdb
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

    # Sleep for 3 sec to ensure that the MIP daemons are ready
    time.sleep(3)

    # Start Routing Daemon on Node A and Node B without gdb
    terms.append(openTerm(self,
                          node=A,
                          title="ROUTING A",
                          geometry="80x14+0+210",
                          cmd="./routingd -d usockA"))

    terms.append(openTerm(self,
                          node=B,
                          title="ROUTING B",
                          geometry="80x14+0+660",
                          cmd="./routingd -d usockB"))

    # Give time for routing daemons to converge
    time.sleep(15)

    # Start client on Node A
    terms.append(openTerm(self,
                          node=A,
                          title="Client [A]",
                          geometry="80x14+0+900",
                          cmd="./ping_client usockA 20 \"Hello from A\""))

def init_twonode_with_client_and_server(self, line):
    "Initialize both nodes with a client on Node A and a server on Node B"
    net = self.mn
    A = net.get('A')
    B = net.get('B')

    # Start MIP Daemon on Node A and Node B
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

    # Sleep for 3 sec to ensure that the MIP daemons are ready
    time.sleep(3)

    # Start Routing Daemon on Node A and Node B
    terms.append(openTerm(self,
                          node=A,
                          title="ROUTING A",
                          geometry="80x14+0+210",
                          cmd="./routingd -d usockA"))

    terms.append(openTerm(self,
                          node=B,
                          title="ROUTING B",
                          geometry="80x14+0+660",
                          cmd="./routingd -d usockB"))

    # Give time for routing daemons to converge
    time.sleep(15)

    # Start a server on Node B
    terms.append(openTerm(self,
                          node=B,
                          title="Server [B]",
                          geometry="80x14+0+900",
                          cmd="./ping_server usockB"))

    # Start a client on Node A
    terms.append(openTerm(self,
                          node=A,
                          title="Client [A]",
                          geometry="80x14+0+1200",
                          cmd="./ping_client usockA 20 \"Hello from A to B\""))


# Mininet Callbacks

# Inside mininet console run `init_twonode_with_gdb`, `init_twonode_without_gdb`, or `init_twonode_with_client`
CLI.do_init_twonode_with_gdb = init_twonode_with_gdb
CLI.do_init_twonode_without_gdb = init_twonode_without_gdb
CLI.do_init_twonode_with_client = init_twonode_with_client
CLI.do_init_twonode_with_client_and_gdb = init_twonode_with_client_and_gdb
CLI.do_init_twonode_with_client_and_server = init_twonode_with_client_and_server

# Inside mininet console run 'EOF' to gracefully kill the mininet console
orig_EOF = CLI.do_EOF

# Kill mininet console
def do_EOF(self, line):
    for t in terms:
        os.kill(t.pid, signal.SIGKILL)
    return orig_EOF(self, line)

CLI.do_EOF = do_EOF

# Topologies
topos = {
    'twonode': (lambda: TwoNodeTopo())
}
