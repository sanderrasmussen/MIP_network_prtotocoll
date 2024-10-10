from mininet.topo import Topo

# topology
# A-------------------------B-------------------------------C

# Usage example:
# sudo -E mn --custom he1-mn-topo.py --topo he1 --link tc --x

class He1Topo( Topo ):
    "Simple topology for Home exam 1."

    def __init__( self ):
        "Set up our custom topo."

        # Initialize topology
        Topo.__init__( self )

        # Add hosts 
        A = self.addHost('A')
        B = self.addHost('B')
        C = self.addHost('C')

        # Add links
        self.addLink(A, B, bw=10, delay='10ms', loss=0.0, use_tbf=False)
        self.addLink(B, C, bw=10, delay='10ms', loss=0.0, use_tbf=False)

topos = { 'he1': ( lambda: He1Topo() ) }
