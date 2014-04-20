RUBY = ruby
SCRIPT = quagga.cli2json.rb
SRCDIR = ../../src/quagga-0.99.22

FILES = bgpd/bgp_debug.c bgpd/bgp_filter.c bgpd/bgp_mplsvpn.c \
	bgpd/bgp_nexthop.c bgpd/bgp_route.c bgpd/bgp_routemap.c \
	bgpd/bgp_vty.c \
	lib/command.c lib/distribute.c lib/filter.c lib/if.c lib/if_rmap.c \
	lib/keychain.c lib/memory.c lib/plist.c lib/routemap.c lib/vty.c \
	ospf6d/ospf6_abr.c ospf6d/ospf6_area.c ospf6d/ospf6_asbr.c \
	ospf6d/ospf6d.c ospf6d/ospf6_flood.c ospf6d/ospf6_interface.c \
	ospf6d/ospf6_intra.c ospf6d/ospf6_lsa.c ospf6d/ospf6_message.c \
	ospf6d/ospf6_neighbor.c ospf6d/ospf6_route.c ospf6d/ospf6_spf.c \
	ospf6d/ospf6_top.c ospf6d/ospf6_zebra.c \
	ospfd/ospf_dump.c ospfd/ospf_opaque.c ospfd/ospf_routemap.c \
	ospfd/ospf_te.c ospfd/ospf_vty.c \
	ripd/ripd.c ripd/rip_debug.c ripd/rip_interface.c ripd/rip_offset.c \
	ripd/rip_routemap.c ripd/rip_routemap.c \
	ripngd/ripngd.c	ripngd/ripng_debug.c ripngd/ripng_interface.c \
	ripngd/ripng_offset.c ripngd/ripng_routemap.c ripngd/ripng_zebra.c \
	zebra/interface.c zebra/irdp_interface.c zebra/rtadv.c \
	zebra/zebra_routemap.c zebra/zebra_vty.c 

SRCS = $(addprefix $(SRCDIR)/,$(FILES))
CLIJSON = $(notdir $(SRCS:.c=.cli.json))

all:
	$(RUBY) $(SCRIPT) $(SRCS)

clean:
	rm -f $(CLIJSON)




