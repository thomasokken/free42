/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2012  Thomas Okken
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see http://www.gnu.org/licenses/.
 *****************************************************************************/

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <unistd.h>
#include <stdarg.h>
#include <pthread.h>

#import "HTTPServerView.h"
#import "shell_iphone.h"

// From simpleserver.c
void handle_client(int csock);
void errprintf(const char *fmt, ...);

static HTTPServerView *instance;
static bool mustStop;
static in_addr_t ip_addr;
static NSString *hostname;
static int port;

@implementation HTTPServerView

@synthesize doneButton;
@synthesize urlLabel;
@synthesize logView;

- (id)initWithFrame:(CGRect)frame {
    if (self = [super initWithFrame:frame]) {
        // Initialization code
    }
    return self;
}

- (void)drawRect:(CGRect)rect {
    // Drawing code
}

static pthread_t getHostNameThread;
static struct sockaddr_in *sa;

static void *getHostName(void *dummy) {
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	struct hostent *h = gethostbyaddr(&sa->sin_addr, sizeof(sa->sin_addr), AF_INET);
	if (h == NULL) {
		int err = h_errno;
		const char *s;
		switch (err) {
			case HOST_NOT_FOUND: s = "HOST_NOT_FOUND"; break;
			case TRY_AGAIN: s = "TRY_AGAIN"; break;
			case NO_RECOVERY: s = "NO_RECOVERY"; break;
			case NO_DATA: s = "NO_DATA"; break;
			default: s = "unexpected error code"; break;
		}
		NSLog(@"Could not determine my DNS hostname: %s", s);
		
	} else {
		NSLog(@"My DNS hostname appears to be %s", h->h_name);
		hostname = [[NSString stringWithCString:h->h_name encoding:NSUTF8StringEncoding] retain];
	}
	
	[pool release];
	static int result = 0;
	return &result;
}

- (void) awakeFromNib {
	ip_addr = 0;

	[logView setFont:[UIFont fontWithName:@"CourierNewPSMT" size:12]];
	
	struct ifaddrs *list;
	if (getifaddrs(&list) == 0) {
		bool first = true;
		for (struct ifaddrs *item = list; item != NULL; item = item->ifa_next) {
			if (item->ifa_addr->sa_family == AF_INET) {
				if (first) {
					// Skip the first interface; that's the loopback adapter
					first = false;
					continue;
				}
				sa = (struct sockaddr_in *) item->ifa_addr;
				ip_addr = sa->sin_addr.s_addr;
				hostname = [[NSString stringWithFormat:@"%d.%d.%d.%d",
							 ip_addr & 255,
							 (ip_addr >> 8) & 255,
							 (ip_addr >> 16) & 255,
							 (ip_addr >> 24) & 255] retain];
				NSLog(@"My IP address appears to be %@", hostname);
				pthread_create(&getHostNameThread, NULL, getHostName, NULL);
				break;
			}
		}
		freeifaddrs(list);
	}
}

- (void) raised {
	instance = self;
	mustStop = false;
	[urlLabel setText:@"(not running)"];
	[logView setText:@""];
	[self performSelectorInBackground:@selector(start_simple_server) withObject:NULL];
	[UIApplication sharedApplication].idleTimerDisabled = YES;
}

- (void) displayHostAndPort {
	if (port == 0) {
		[urlLabel setText:@"(not running)"];
	} else {
		[urlLabel setText:[NSString stringWithFormat:@"http://%@:%d/", hostname, port]];
	}
}

- (IBAction) done {
	[UIApplication sharedApplication].idleTimerDisabled = NO;
	if (port != 0) {
		mustStop = true;
		int sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock != -1) {
			struct sockaddr_in sa;
			sa.sin_family = AF_INET;
			sa.sin_port = htons(port);
			sa.sin_addr.s_addr = ip_addr;
			connect(sock, (struct sockaddr *) &sa, sizeof(sa));
			close(sock);
		}
	}		
	[shell_iphone showMain];
}

- (void)dealloc {
    [super dealloc];
}

- (void) start_simple_server {
	NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
	
	port = 9090;
    int backlog = 32;
    int ssock, csock;
    struct sockaddr_in sa, ca;
    int err;
	
    ssock = socket(AF_INET, SOCK_STREAM, 0);
    if (ssock == -1) {
		err = errno;
		errprintf("Could not create socket: %s (%d)\n", strerror(err), err);
		goto done;
    }
	
    sa.sin_family = AF_INET;
    sa.sin_port = htons(port);
    sa.sin_addr.s_addr = INADDR_ANY;
    err = bind(ssock, (struct sockaddr *) &sa, sizeof(sa));
    if (err != 0) {
		err = errno;
		errprintf("Could not bind socket to port %d: %s (%d)\n", port, strerror(err), err);
		goto done;
    }
	
    err = listen(ssock, backlog);
    if (err != 0) {
		err = errno;
		errprintf("Could not listen (backlog = %d): %s (%d)\n", backlog, strerror(err), err);
		goto done;
    }
	
	errprintf("The server is listening on port %d.\n", port);
	[instance performSelectorOnMainThread:@selector(displayHostAndPort) withObject:nil waitUntilDone:NO];
	
    while (1) {
		unsigned int n = sizeof(ca);
		char cname[256];
		csock = accept(ssock, (struct sockaddr *) &ca, &n);
		if (csock == -1) {
			err = errno;
			errprintf("Could not accept connection from client: %s (%d)\n", strerror(err), err);
			goto done;
		}
		if (mustStop) {
			close(csock);
			goto done;
		}
		inet_ntop(AF_INET, &ca.sin_addr, cname, sizeof(cname));
		errprintf("Accepted connection from %s\n", cname);
		// TODO: call handle_client() on the main thread,
		// so we don't have to deal with thread safety there
		handle_client(csock);
    }
	
done:
	port = 0;
	if (ssock != -1)
		close(ssock);
	[instance performSelectorOnMainThread:@selector(displayHostAndPort) withObject:nil waitUntilDone:NO];
	[pool release];
}

- (void) appendToLog:(NSString *) text {
	[logView setText:[[logView text] stringByAppendingString:text]];
	NSRange r;
	r.location = [[logView text] length];
	r.length = 0;
	[logView scrollRangeToVisible:r];
}

void errprintf(const char *fmt, ...) {
	va_list ap;
    char text[1024];
    va_start(ap, fmt);
    vsprintf(text, fmt, ap);
	[instance performSelectorOnMainThread:@selector(appendToLog:) withObject:[NSString stringWithCString:text encoding:NSUTF8StringEncoding] waitUntilDone:NO];
	va_end(ap);
}

@end
