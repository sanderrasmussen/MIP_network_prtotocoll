#!/usr/bin/env python

""" mininet script to test IN4230 HE2 assignments"""

from mininet.topo import Topo

from mininet.cli import CLI
# from mininet.term import makeTerm
from mininet.term import tunnelX11
import os
import signal
import time

# Usage example:
# sudo -E mn --mac --custom he2-mn-script.py --topo he2 --link tc


class HE2Topo(Topo):
    "Larger topology for home exams."

    def __init__(self):
        "Set up our custom topo."

        # Initialize topology
        Topo.__init__(self)

        # Add hosts
        A = self.addHost('A')
        B = self.addHost('B')
        C = self.addHost('C')
        D = self.addHost('D')
        E = self.addHost('E')

        # Add links. Note the packet loss of 1% (~1 out of 10 packets is discarded)
        self.addLink(A, B, bw=10, delay='10ms', loss=1.0, use_tbf=False)
        self.addLink(B, C, bw=10, delay='10ms', loss=1.0, use_tbf=False)
        self.addLink(B, D, bw=10, delay='10ms', loss=1.0, use_tbf=False)
        self.addLink(C, D, bw=10, delay='10ms', loss=1.0, use_tbf=False)
        self.addLink(D, E, bw=10, delay='10ms', loss=1.0, use_tbf=False)


terms = []


def openTerm(self, node, title, geometry, cmd="bash"):
    display, tunnel = tunnelX11(node)
    return node.popen(["xterm",
                       "-hold",
                       "-title", "'%s'" % title,
                       "-geometry", geometry,
                       "-display", display,
                       "-e", cmd])


def init_he2(self, line):
    "init is an example command to extend the Mininet CLI"
    net = self.mn
    A = net.get('A')
    B = net.get('B')
    C = net.get('C')
    D = net.get('D')
    E = net.get('E')

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

    terms.append(openTerm(self,
                          node=C,
                          title="MIP C",
                          geometry="80x14+555+0",
                          cmd="./mipd -d usockC 30"))

    terms.append(openTerm(self,
                          node=D,
                          title="MIP D",
                          geometry="80x14+1110+450",
                          cmd="./mipd -d usockD 40"))

    terms.append(openTerm(self,
                          node=E,
                          title="MIP E",
                          geometry="80x14+1110+0",
                          cmd="./mipd -d usockE 50"))

    # Sleep for 3 sec. to make sure that the MIP daemons are ready
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

    terms.append(openTerm(self,
                          node=C,
                          title="ROUTING C",
                          geometry="80x14+555+210",
                          cmd="./routingd -d usockC"))

    terms.append(openTerm(self,
                          node=D,
                          title="ROUTING D",
                          geometry="80x14+1110+660",
                          cmd="./routingd -d usockD"))

    terms.append(openTerm(self,
                          node=E,
                          title="ROUTING E",
                          geometry="80x14+1110+210",
                          cmd="./routingd -d usockE"))

    # Make sure that the MIP and Routing daemons are ready and the topology
    # is converged
    time.sleep(15)

    # (1) Launch ping_server at Node E
    terms.append(openTerm(self,
                          node=E,
                          title="SERVER [E]",
                          geometry="38x20+807+583",
                          cmd="./ping_server usockE"))

    time.sleep(3)

    # (2) Ping from Node A to Node E with TTL 8
    terms.append(openTerm(self,
                          node=A,
                          title="CLIENT [A]",
                          geometry="38x20+555+583",
                          cmd="./ping_client usockA 50 \"Hello from A\" 8"))

    time.sleep(3)

    # (3) Ping from Node C to Node E with TTL 8
    terms.append(openTerm(self,
                          node=C,
                          title="CLIENT [C]",
                          geometry="38x20+555+583",
                          cmd="./ping_client usockC 50 \"Hello from C\" 8"))

    time.sleep(3)

    # (4) Ping from A with TTL = 1 should be discarded and generate a timeout
    terms.append(openTerm(self,
                          node=A,
                          title="CLIENT [A]",
                          geometry="38x20+555+583",
                          cmd="./ping_client usockA 50 \"Hello with TTL 1\" 1"))

    time.sleep(3)

    # (5) Ping from C with TTL = 3 should make it
    terms.append(openTerm(self,
                          node=C,
                          title="CLIENT [C]",
                          geometry="38x20+555+583",
                          cmd="./ping_client usockC 50 \"Hello with TTL 3\" 3"))

    time.sleep(30)

    # After 30 sec. fail the link betwen node B and D for 20 sec.
    # DVR should be able to find another shortest path and reroute the packets
    # from A to E through C

    net.configLinkStatus('B', 'D','down')
    time.sleep(20)

    # (6) Ping from A with default TTL = 8
    terms.append(openTerm(self,
                          node=A,
                          title="CLIENT [A]",
                          geometry="38x20+555+583",
                          cmd="./ping_client usockA 50 \"Hello from A\" 8"))

    # Bring the link up again. The network should converge again and use
    # A - B - D - E as the shortest path
    net.configLinkStatus('B', 'D','up')
    time.sleep(20)

    # (7) Ping again from A with default TTL = 8
    terms.append(openTerm(self,
                          node=A,
                          title="CLIENT [A]",
                          geometry="38x20+555+583",
                          cmd="./ping_client usockA 50 \"Hello from A\" 8"))


# Mininet Callbacks

# Inside mininet console run `init_he2`
CLI.do_init_he2 = init_he2

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
    'he2': (lambda: HE2Topo())
}
