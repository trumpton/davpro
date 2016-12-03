//
//  XYZ Davinci Management
//
//  This is a command line programme, which is used to interrogate the XYZ Davinci printer, and submit jobs.
//
//  Current status: CONCEPT
//
//  What Works:
//
//    Status acquisition, and much of the parsing (in both plaintext and json)
//
//  What Doesn't:
//
//    There is no database of filament cartridge codes (if that is how the printer knows ABS / Colour etc)
//    Submitting prints hasn't been coded yet
//    Better handling of flakey network connections is needed
//
//  Not Sure About:
//    Not all of the status is decoded
//    Not sure how multiple extruders are represented (I only have one)
//    Not sure what other Davinci models outputs look like (I've got a pro)
//
//  Try Me:
//
//    g++ davpro.c -o davpro
//    ./davpro -d 192.168.1.7 9100
//    ./davpro -j 192.168.1.7 9100
//    ./davpro -v 192.168.1.7 9100
//
//  2016-08-22 S M Clarke
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include <cstring>
#include <string>

// Max Number of Nozzles
#define NOZZLES 2

// Printer Interface Functions
int getrawstatus(char *ipaddress, int port) ;
int getstatus(char *ipaddress, int port) ;
int getdemo(char *ipaddress, int port) ;
int uploadfile(char *ipaddress, int port, char *filename) ;

// Network Functions
int openandconnect(std::string ipaddress, int port) ;
int readchar(int sockfd) ;
std::string readline(int sockfd) ;

////////////////////////////////////////////////////
//
// Main
//
int main(int argc, char *argv[])
{
    int sockfd ;
    int status ;

    if ((argc<4 || argc>5) || 
	(argc==4 && std::string(argv[1])!=std::string("-j") && 
		std::string(argv[1])!=std::string("-v") &&
		std::string(argv[1])!=std::string("-d")) || 
	(argc==5 && std::string(argv[1])!=std::string("-u"))) {
      fprintf(stderr, "davpro -juv ipaddress port [ uploadfile.3w ]\n") ;
      fprintf(stderr, "  j: json query, u: upload file, v: raw verbose\n") ;
      exit(-1) ;
    }

    switch (argv[1][1]) {
      case 'j':
	status = getstatus(argv[2], atoi(argv[3])) ;
	break ;
      case 'v':
	status = getrawstatus(argv[2], atoi(argv[3])) ;
        break ;
      case 'd':
	status = getdemo(argv[2], atoi(argv[3])) ;
        break ;
      case 'u':
	status = uploadfile(argv[2], atoi(argv[3]), argv[4]) ;
        break ;
    }
    return status;
}


////////////////////////////////////////////////////
//
// Status Function
//

std::string status ;
int statuscode ;

int getrawstatus(char *ipaddress, int port)
{
    int sockfd ;
    std::string buffer ;
    int n ;

    sockfd = openandconnect(std::string(ipaddress), port) ;
    if (sockfd >= 0) {

      // Write Command
      buffer="XYZv3/query=a\r\n" ;
      n = write(sockfd,buffer.c_str(),buffer.size());

      if (n < 0) {
	status  = "error: unable to write to printer" ;
	statuscode = -4 ;

      } else do {

        // Get Response
        buffer = readline(sockfd) ;
        if (buffer.length()>2 ) printf("%s\n", buffer.c_str()) ;

      } while (buffer.length()>=1 && buffer.at(0)!='E' && buffer.at(0)!='$') ;

      close(sockfd) ;
    }

    return statuscode ;	
}


int getstatus(char *ipaddress, int port)
{
    std::string buffer ;
    int  n;

    std::string serial, model, language ;
    float nozzletemp[NOZZLES], filament[NOZZLES], nozzleontime[NOZZLES], nozzleruntime[NOZZLES] ;
    std::string cartridgetype[NOZZLES] ;
    bool xyzfilament[NOZZLES] ;
    std::string firmwareversion, wifiversion ;
    int lastnozzle=0 ;
    float bedtemp=-1 ;
    bool dooropen=true, lidopen=true ;
    bool idle=false, printing=false ;
    int sockfd ;
    int percentcomplete=0 ;
    int elapsedmins=0 ;
    int remainingmins=0 ;

    serial="unknown" ;
    model="unknown" ;
    language="unknown" ;
    firmwareversion="unknown" ;
    wifiversion="unknown" ;

    for (int i=0; i<NOZZLES; i++) {
	nozzletemp[i]=-1 ; filament[i]=-1 ; nozzleontime[i]=-1 ; nozzleruntime[i]=-1 ;
        cartridgetype[i]="unknown" ;
	xyzfilament[i]=false ;
    }

    status="unknown" ;
    statuscode = 0 ;

    sockfd = openandconnect(std::string(ipaddress), port) ;
    if (sockfd >= 0) {

      // Write Command
      buffer="XYZv3/query=a\r\n" ;
      n = write(sockfd,buffer.c_str(),buffer.size());

      if (n < 0) {

	status = "error: unable to write to printer" ;
	statuscode = -4 ;

      } else do {

        // Get Response
        buffer = readline(sockfd) ;
        if (buffer.length()>2) {

	  // Collect / interpret response
	  std::string data = buffer.substr(2) ;
	  switch (buffer.at(0)) {
	  case 'b': { // b:26
		      // Bed Temperature
			sscanf(data.c_str(), "%f", &bedtemp) ;
		} break ;
	  case 'd': { // d:67,5,3
		      // % Complete, Minutes Elapsed, Total Minutes
		      int c1 = data.find(',') ;
		      int c2 = data.find(',', c1+1) ;
		      percentcomplete = atoi(data.substr(0,c1).c_str()) ;
		      elapsedmins = atoi(data.substr(c1+1, c2-c1).c_str()) ;
		      remainingmins = atoi(data.substr(c2+1).c_str()) ;
		} break ;
	  case 'e': { // e:0
		} break ;
	  case 'f': { // f:1,200000
		      // It's a guess that multiple nozzles appear as multiple entries
		      int comma = data.find(',') ;
		      int nozznum = atoi(data.substr(0, comma).c_str()) ;
		      if (nozznum>=0 && nozznum<NOZZLES) {
			sscanf(data.substr(comma+1).c_str(), "%f", &filament[nozznum-1]) ;
			filament[nozznum-1] /= 1000.0 ;
			if (lastnozzle<nozznum) lastnozzle=nozznum ;
		      }
		} break ;
	  case 'i':   // i:9Q7KEPAZ5DE12KA401
		      serial = data ;
		break ;
	  case 'j': // j:9511
		statuscode = atoi(data.c_str()) ;
		switch (atoi(data.c_str())) {
		case 9511:
		case 9700:
			idle = true ;
			printing = false ;
			status = "Idle" ;
			break ;
		case 9500:
			status = "Initialising" ;
			idle = false ;
			printing = true ;
			break ;
		case 9501:
			status = "Heating" ;
			idle = false ;
			printing = true ;
			break ;
		case 9601:
			status = "Paused" ;
			idle = false ;
			printing = true ;
			break ;
		case 9505:
			status = "Printing" ;
			idle = false ;
			printing = true ;
			break ;
		case 9506:
			status = "Cooling" ;
			idle = false ;
			printing = true ;
			break ;
		case 9508:
			status = "Resetting" ;
			idle = false ;
			printing = true ;
			break ;
		case 9509:
			status = "Complete, Press OK" ;
			idle = false ;
			printing = true ;
			break ;
		default:
			status = std::string("Unknown: ") + data ;
			idle = false ;
			printing = false ;
			break ;
		}
		break ;
	  case 'L': { // L:1,50010,20722
		      // % Nozzle, Ontime, Runtime(min)
		      int c1 = data.find(',') ;
		      int c2 = data.find(',', c1+1) ;
		      int noz = atoi(data.substr(0,c1).c_str()) ;
		      if (noz>=1 && noz<=NOZZLES) {
			nozzleontime[noz-1] = atoi(data.substr(c1+1, c2-c1).c_str()) ;
		        nozzleruntime[noz-1] = atoi(data.substr(c2+1).c_str()) ;
		      }
		}
		break ;
	  case 'm': // m:0,0,0
		break ;
	  case 'o': // o:p8,t1,c1,a-
		break ;
	  case 'O': // O:{"nozzle":"230","bed":"90"}
		break ;
	  case 'p': // p:dvF1W0A000
		break ;
	  case 's': { // s:{"fm":1,"fd":1,"dr":{"top":"off","front":"off"},"sd":"yes","eh":"0","of":"1"}
		    // fm / fd - associated with whether filament is in by drive or by extruder - not sure why this isn't per extruder
		    // dr - door status (open / closed)
		    // sd -
		    // eh - 
		    // of -

			// Door and Lid Status: Search for "dr":, and extract {}
			std::string doorsearch = "\"dr\":" ;
			std::string topsearch = "\"top\":\"" ;
			std::string frontsearch = "\"front\":\"" ;
			int doorpos = data.find(doorsearch) ;
			if (doorpos>0) {
				std::string door = data.substr(doorpos) ;
				// Search for "top" and extract lid status
				int toppos = door.find(topsearch) + topsearch.length() ;
				if (toppos>0) {
					std::string topoff = door.substr(toppos, door.substr(toppos).find("\"")) ;
					if (topoff.compare("on")) lidopen=false ;
				}
				// Search for "front" and extract door status
				int doorpos = door.find(frontsearch) + frontsearch.length() ;
				if (doorpos>0) {
					std::string dooroff = door.substr(doorpos, door.substr(doorpos).find("\"")) ;
					if (dooroff.compare("on")) dooropen=false ;
				}
			}
		} break ;

	  case 't': { // t:1,22
			// It's a guess that multiple nozzles appear as multiple entries
			int comma = data.find(',') ;
			int nozznum = atoi(data.substr(0, comma).c_str()) ;
			if (nozznum>=0 && nozznum<NOZZLES) {
				sscanf(data.substr(comma+1).c_str(), "%f", &nozzletemp[nozznum-1]) ;
				if (lastnozzle<nozznum) lastnozzle=nozznum ;
			}
		} break ;
		break ;
	  case 'v': { // v:1.2.8
			firmwareversion = data ;
		} break ;
	  case 'w': { // w:1,--------------
			// It's a guess that multiple cartridges appear as multiple entries
			int comma = data.find(',') ;
			int nozznum = atoi(data.substr(0, comma).c_str()) ;
			if (nozznum>=0 && nozznum<NOZZLES)
				cartridgetype[nozznum-1] = data.substr(comma+1) ;
				if (cartridgetype[nozznum-1].length()>1 && cartridgetype[nozznum-1].at(0) == '-') 
					cartridgetype[nozznum-1] = "notinstalled" ;
				else
					xyzfilament[nozznum-1] = true ;
				if (lastnozzle<nozznum) lastnozzle=nozznum ;
		} break ;
	  case 'n': { // n:daVinci Pro. 1w 
			model = data ;
		} break ;
	  case 'X': // X:2,GB-0004-0000-TH-000-0000-00000
		break ;
	  case 'l': { // l:en
			language = data ;
		} break ;
	  case 'V': { // V:5.1.5
			wifiversion = data ;
		} break ;
	  case '4': // 4:{"wlan":{"ip":"192.168.1.2","ssid":"Network-SSID","channel":"1","MAC":"20:f8:00:00:00:00"}}
		break ;
	  case 'E': // Error
		statuscode = 9999 ;
		status = std::string("Busy : ") + buffer.substr(1, buffer.find('$')).c_str() ;
		idle = false ;
		printing = false ;
  	  default:
		break ;
	  }
        }
      } while (buffer.length()>1 && buffer.at(0)!='E' && buffer.at(0)!='$') ;

      if (buffer.length()==0) {
	status = "error: unable to read from printer, unrecognised data returned" ;
        statuscode = -5 ;
      }

    }

    if (sockfd>=0) {
	close(sockfd) ;
    }

    printf("{\n") ;

    if (statuscode!=9999 && statuscode>=0) {

      printf("  \"model\" : \"%s\",\n", model.c_str()) ;
      printf("  \"serial\" : \"%s\",\n", serial.c_str()) ;
      printf("  \"bedtemperature\" : %.1f,\n", bedtemp) ;
      printf("  \"nozzles\" : %d,\n", lastnozzle) ;
      printf("  \"percentcomplete\" : %d,\n", percentcomplete) ;
      printf("  \"remainingmins\" : %d,\n", remainingmins) ;
      printf("  \"elapsedmins\" : %d,\n", elapsedmins) ;
      if (lastnozzle>0) {
	printf("  \"filamentcartridgeid\" : { ") ;
	for (int i=0; i<lastnozzle; i++) {
		printf("\"%d\" : \"%s\"", i+1, cartridgetype[i].c_str()) ;
		if (i<(lastnozzle-1)) printf(", ") ;
		else printf(" },\n") ;
	}
	printf("  \"xyzfilament\" : { ") ;
	for (int i=0; i<lastnozzle; i++) {
		printf("\"%d\" : \"%s\"", i+1, xyzfilament[i]?"true":"false") ;
		if (i<(lastnozzle-1)) printf(", ") ;
		else printf(" },\n") ;
	}
	printf("  \"nozzletemperature\" : { ") ;
	for (int i=0; i<lastnozzle; i++) {
		printf("\"%d\" : %.1f", i+1, nozzletemp[i]) ;
		if (i<(lastnozzle-1)) printf(", ") ;
		else printf(" },\n") ;
	}
	printf("  \"filamentremaining\" : { ") ;
	for (int i=0; i<lastnozzle; i++) {
		printf("\"%d\" : %.3f", i+1, filament[i]) ;
		if (i<(lastnozzle-1)) printf(", ") ;
		else printf(" },\n") ;
	}
	printf("  \"ontime\" : { ") ;
	for (int i=0; i<lastnozzle; i++) {
		printf("\"%d\" : %.0f", i+1, nozzleontime[i]) ;
		if (i<(lastnozzle-1)) printf(", ") ;
		else printf(" },\n") ;
	}
	printf("  \"runtime\" : { ") ;
	for (int i=0; i<lastnozzle; i++) {
		printf("\"%d\" : %.0f", i+1, nozzleruntime[i]) ;
		if (i<(lastnozzle-1)) printf(", ") ;
		else printf(" },\n") ;
	}
	
      }
      printf("  \"dooropen\" : %s,\n", (dooropen?"true":"false")) ;
      printf("  \"lidopen\" : %s,\n", (lidopen?"true":"false")) ;
      printf("  \"language\" : \"%s\",\n", language.c_str()) ;
      printf("  \"ipaddress\" : \"%s:%d\",\n", ipaddress, port) ;
      printf("  \"version\" : { \"firmware\" : \"%s\", \"wifi\" : \"%s\" },\n", 
	firmwareversion.c_str(), wifiversion.c_str()) ;
    }

    printf("  \"status\" : \"%s\",\n", status.c_str()) ;
    printf("  \"statuscode\" : %d,\n", statuscode) ;
    printf("  \"busy\" : %s,\n", statuscode==9999?"true":"false") ;
    printf("  \"idle\" : %s,\n", idle?"true":"false") ;
    printf("  \"printing\" : %s,\n", printing?"true":"false") ;
    printf("  \"error\" : %s\n", (statuscode<0)?"true":"false") ;

    printf("}\n") ;

    return statuscode ;
}

int getdemo(char *ipaddress, int port)
{
    printf("{\n") ;
    printf("  \"model\" : \"Davinci Pro\",\n") ;
    printf("  \"serial\" : \"GB121FF34IEE2\",\n") ;
    printf("  \"status\" : \"Printer Idle\",\n") ;
    printf("  \"statuscode\" : 0,\n") ;
    printf("  \"printing\" : true,\n") ;
    printf("  \"idle\" : false,\n") ;
    printf("  \"bedtemperature\" : 32.1,\n") ;
    printf("  \"percentcomplete\" : 20,\n") ;
    printf("  \"elapsedmins\" : 5,\n") ;
    printf("  \"remainingmins\" : 40,\n") ;
    printf("  \"nozzles\" : 2,\n") ;
    printf("  \"filamentcartridgeid\" : { \"1\" : \"E323IAAU3\", \"2\" : \"unknown\" },\n") ;
    printf("  \"xyzfilament\" : { \"1\" : true, \"2\" : false },\n") ;
    printf("  \"nozzletemperature\" : { \"1\" : 160.1, \"2\" : 23.1 },\n") ;
    printf("  \"filamentremaining\" : { \"1\" : 122.321, \"2\" : 20.9 },\n") ;
    printf("  \"ontime\" : { \"1\" : 123221, \"2\" : 132101 },\n") ;
    printf("  \"runtime\" : { \"1\" : 60432, \"2\" : 43121 },\n") ;
    printf("  \"dooropen\" : false,\n") ;
    printf("  \"lidopen\" : false,\n") ;
    printf("  \"language\" : \"en\",\n") ;
    printf("  \"ipaddress\" : \"%s:%d\",\n", ipaddress, port) ;
    printf("  \"version\" : { \"firmware\" : \"1.3.2\", \"wifi\" : \"1.5.5\" }\n") ;
    printf("}\n") ;
    return 0 ;
}

////////////////////////////////////////////////////
//
// Upload Function
//
int uploadfile(char *ipaddress, int port, char *filename)
{
    int fi;

    statuscode=0 ;
    status = "ok" ;

    fi = open(filename, O_RDONLY) ;
    if (fi<0) {

	status = "error: file not found" ;
	statuscode = -1 ;

    } else {

	int n ;
	int sockfd ;
        char buffer[1024] ;

	sockfd = openandconnect(std::string(ipaddress), port) ;

        if (sockfd >= 0) {

	    // TODO: Calculate size of file
	    long filesize = 2048 ;
	    long twentieth = (filesize/20)-1 ;

	    // Write Command
	    strcpy(buffer, "XYZv3/upload=MyTest.gcode,5120\r\n") ; // 25040
	    n = write(sockfd,buffer,strlen(buffer));

	    if (n < 0) {

		status = "error: unable to write to printer" ;
		statuscode = -4 ;

	    } else {

/*

The following code causes a VPN to crash / drop.  The following information is left in the 
syslog file ...

  Sep  7 18:02:33 dime pptp[32457]: nm-pptp-service-32446 warn[decaps_gre:pptp_gre.c:347]: short read (-1): Message too long
  Sep  7 18:02:33 dime pptp[32464]: nm-pptp-service-32446 log[callmgr_main:pptp_callmgr.c:245]: Closing connection (unhandled)

*/

		long buflen = sizeof(buffer) ;
		if (twentieth < buflen && twentieth > 2) buflen = twentieth - 1 ;

		long count = twentieth ;

		while ( statuscode==0 && (n = read(fi, buffer, buflen)) >0 ) {
			if (write(sockfd, buffer, n)!=n) {
				status = "error: unable to write to printer" ;
				statuscode = -4 ;
			}
			count -= n ;
			if (count<0) {
				printf(".") ;
				count+=twentieth ;
			}
		}

		if (statuscode==0) {
			strcpy(buffer, "XYZv3/uploadDidFinish\r\n") ; 
			n = write(sockfd,buffer,strlen(buffer));
			printf(".") ;
		}

	    }

	    close(sockfd) ;
	}

	close(fi) ;
   }

  printf("\n%s\n", status.c_str()) ;
  return statuscode ;
}


////////////////////////////////////////////////////
//
// Cartridge Types
//
typedef struct struct_cartridgeinfo {
  char *cartridgeid ;
  char *material ;
  char *colour ;
  float filamenttemp ;
  float bedtemp ;
  struct struct_cartridgeinfo *next ;
} cartridgeinfo ;

cartridgeinfo *cartridges ;

////////////////////////////////////////////////////
//
// Network Functions
//

int openandconnect(std::string ipaddress, int port)
{
    int sockfd ;
    struct sockaddr_in serv_addr;
    struct timeval tv ;

    // Create a Non-Blocking Socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
	status = "error, unable to create a socket" ;
        statuscode = -1 ;
        return -1 ;
    } else {
	int arg = fcntl(sockfd, F_GETFL, NULL); 
	arg |= O_NONBLOCK; 
	fcntl(sockfd, F_SETFL, arg); 
    }

    // Connect to the IP Address
    memset((char *) &serv_addr, '\0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    inet_aton(ipaddress.c_str(), &serv_addr.sin_addr);
    serv_addr.sin_port = htons(port);
    connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) ;
 
    // Wait for connection to be established
    fd_set myset ;
    int valopt ;
    tv.tv_sec = 8;
    tv.tv_usec = 0; 
    FD_ZERO(&myset); 
    FD_SET(sockfd, &myset); 
    if (select(sockfd+1, NULL, &myset, NULL, &tv) <= 0) { 
      status = "error: timeout connecting to printer" ; 
      statuscode = -3 ;
      return -3 ;
    } else { 
      socklen_t lon ;
      int valopt ;
      lon = sizeof(int); 
      getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon); 
      if (valopt) { 
         status = std::string("error: unable to connect to printer - ") + std::string(strerror(valopt)); 
	 statuscode = -3 ;
	 return -3 ;
      }
    }

    // Set blocking 
    fcntl(sockfd, F_SETFL,fcntl(sockfd, F_GETFL, NULL)^O_NONBLOCK) ; 

    return sockfd ;
}

std::string readline(int sockfd)
{
  int i=0 ;
  int ch ;
  std::string buffer ;
  do {
    ch=readchar(sockfd) ;
    if (ch!='\r') {
      if (ch>0 && ch!='\n') buffer = buffer + (char)ch ;
    }
  } while (ch>0 && ch!='\n') ;
  return buffer ;
}

int readchar(int sockfd)
{
  char buf[1] ;
  switch (read(sockfd, buf, sizeof(buf))) {
  case 1:
    return (buf[0]) ;
    break ;
  case 0:
  default:
    status = "error, unable to read character from printer" ;
    statuscode = -2 ;
    return -2 ;
    break ;
  }
}


/*

Upload Protocol:
----------------

Input
!   Response
v   !
    v
 
00000000  58 59 5a 76 33 2f 75 70  6c 6f 61 64 3d 4d 79 54   XYZv3/up load=MyT
00000010  65 73 74 2e 67 63 6f 64  65 2c 32 35 30 34 30 0d   est.gcod e,25040.
00000020  0a                                                 .
    00000000  6f 6b 0a                                           ok.
00000021  00 00 00 00 00 00 20 00  33 44 50 46 4e 4b 47 31   ...... . 3DPFNKG1
00000031  33 57 54 57 01 05 00 00  00 00 12 4c 00 00 00 00   3WTW.... ...L....
00000041  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00   ........ ........
00000051  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00   ........ ........
...
...
00002015  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00   ........ ........
00002025  00 00 00 00 00 00 00 00                            ........ 
    00000003  6f 6b 0a                                           ok.
0000202D  00 00 00 01 00 00 20 00  64 7d b5 0f 1e 37 da d9   ...... . d}...7..
0000203D  2a 8f 68 8b 79 9a cb 91  28 11 99 9a f8 6a 9a de   *.h.y... (....j..
...
...
0000601D  e2 d2 78 b4 da 8b 94 fa  03 03 ee 0d 58 64 7e 32   ..x..... ....Xd~2
0000602D  fd 50 f2 60 fe d9 65 92  95 c6 7f 0b dd ac ab 65   .P.`..e. .......e
0000603D  1e 0f e6 b7 00 00 00 00                            ........ 
    00000009  6f 6b 0a                                           ok.
00006045  00 00 00 03 00 00 01 d0  81 86 19 4e d6 e8 b4 61   ........ ...N...a
00006055  90 c7 dd 88 8a da 35 89  67 71 bd 87 e2 59 10 bf   ......5. gq...Y..
...
...
00006205  8a ef c4 f9 2e 76 d0 46  73 37 68 cb 43 a2 16 ec   .....v.F s7h.C...
00006215  76 3e 7f fa 51 1f 48 c5  00 00 00 00               v>..Q.H. ....
    0000000C  6f 6b 0a                                           ok.
00006221  58 59 5a 76 33 2f 75 70  6c 6f 61 64 44 69 64 46   XYZv3/up loadDidF
00006231  69 6e 69 73 68                                     inish
    0000000F  6f 6b 0a                                           ok.
    00000012  6f 6b 0a                                           ok.
    00000015  6f 6b 0a                                           ok.
*/

