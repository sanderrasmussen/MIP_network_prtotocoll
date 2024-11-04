#!/usr/bin/env python

"""Mininet script for testing two-node MIP and routing daemons"""

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


def init_twonode(self, line):
    "init is an example command to extend the Mininet CLI"
    net = self.mn
    A = net.get('A')
    B = net.get('B')

    # MIP Daemons
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

    # Routing Daemons
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


# Mininet Callbacks

# Inside mininet console run `init_twonode`
CLI.do_init_twonode = init_twonode

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
