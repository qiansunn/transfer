


from ryu.lib.mac import haddr_to_bin
from ryu.lib.packet import *

def sendip(self, dstip, srcip,  payload=0):
    	"""Send raw IP packet on interface."""

	#create a raw socket
	try:
	    s = socket.socket(socket.AF_INET, socket.SOCK_RAW, socket.IPPROTO_RAW)
	except socket.error , msg:
	    print 'Socket could not be created. Error Code : ' + str(msg[0]) + ' Message ' + msg[1]
	    sys.exit()

	# tell kernel not to put in headers, since we are providing it, when using IPPROTO_RAW this is not necessary
	# s.setsockopt(socket.IPPROTO_IP, socket.IP_HDRINCL, 1)

	# now start constructing the packet
	packet = '';

	# ip header fields
	ip_ihl = 5
	ip_ver = 4
	ip_tos = 0
	ip_tot_len = 0  # kernel will fill the correct total length
	ip_id = min(65535, payload)   #Id of this packet is the number of marks 54321
	ip_frag_off = 0
	ip_ttl = 255
	ip_proto = 143
	ip_check = 0    # kernel will fill the correct checksum
	ip_saddr = socket.inet_aton ( srcip )   #Spoof the source ip address if you want to
	ip_daddr = socket.inet_aton ( dstip)

	ip_ihl_ver = (ip_ver << 4) + ip_ihl

	# the ! in the pack format string means network order
	ip_header = pack('!BBHHHBBH4s4s' , ip_ihl_ver, ip_tos, ip_tot_len, ip_id, ip_frag_off, ip_ttl, ip_proto, ip_check, ip_saddr, ip_daddr)

  	# final full packet - syn packets dont have any data
	packet = ip_header #+ payload
    	#print datetime.now(), '-> Sent marking packet to: ', dstip, ' message : ', packet

	#Send the packet finally - the port specified has no effect
	s.sendto(packet, (dstip , 0 ))    # put this in a loop if you want to flood the target
	s.close()
	#print 'Sent marking packet to: ', dstip, ' from: ', srcip, ' with marking= ', payload