/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2019  Thomas Okken
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
#import "CalcView.h"
#import "Free42AppDelegate.h"
#import "RootViewController.h"

// From simpleserver.c
void handle_client(int csock);
void errprintf(const char *fmt, ...);

static HTTPServerView *instance;
static int pype[2];
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
    [super awakeFromNib];
    ip_addr = 0;

    [logView setFont:[UIFont fontWithName:@"CourierNewPSMT" size:12]];
    
    // TODO: This stuff should be done whenever the HTTP Server view
    // is activated, not just when the app is started. It is possible
    // for the network interfaces to get reconfigured even while the
    // app is running, and it should be able to deal with that.
    
    struct ifaddrs *list;
    if (getifaddrs(&list) == 0) {
        for (struct ifaddrs *item = list; item != NULL; item = item->ifa_next) {
            if (item->ifa_addr->sa_family == AF_INET) {
                {
                    struct sockaddr_in *sa2 = (struct sockaddr_in *) item->ifa_addr;
                    in_addr_t ip_addr2 = sa2->sin_addr.s_addr;
                    NSString *hostname2 = [[NSString stringWithFormat:@"%d.%d.%d.%d",
                                 ip_addr2 & 255,
                                 (ip_addr2 >> 8) & 255,
                                 (ip_addr2 >> 16) & 255,
                                 (ip_addr2 >> 24) & 255] retain];
                    NSString *ipStr2 = [[NSString stringWithString:hostname2] retain];
                    NSLog(@"interface: %s (%@)", item->ifa_name, ipStr2);
                }
                if (strcmp(item->ifa_name, "en0") == 0 || strcmp(item->ifa_name, "en1") == 0) {
                    sa = (struct sockaddr_in *) item->ifa_addr;
                    ip_addr = sa->sin_addr.s_addr;
                    hostname = [[NSString stringWithFormat:@"%d.%d.%d.%d",
                                 ip_addr & 255,
                                 (ip_addr >> 8) & 255,
                                 (ip_addr >> 16) & 255,
                                 (ip_addr >> 24) & 255] retain];
                    ipStr = [[NSString stringWithString:hostname] retain];
                    NSLog(@"My IP address appears to be %@", hostname);
                    pthread_create(&getHostNameThread, NULL, getHostName, NULL);
                    //break;
                }
            }
        }
        freeifaddrs(list);
    }
}

- (void) raised {
    instance = self;
    pipe(pype); // only fails if out of file descriptors
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
        alternateUrl = [[NSString stringWithFormat:@"http://%@:%d/", ipStr, port] retain];
    }
}

- (IBAction) done {
    if (!state.alwaysOn)
        [UIApplication sharedApplication].idleTimerDisabled = NO;
    write(pype[1], "1\n", 2);
    [RootViewController showMain];
}

- (void) touchesBegan: (NSSet *) touches withEvent: (UIEvent *) event {
    if ([touches count] == 1 && CGRectContainsPoint([urlLabel frame], [((UITouch *) [touches anyObject]) locationInView:self])) {
        NSString *temp = [urlLabel text];
        if (alternateUrl != nil && ![temp isEqualToString:@"(not running"]) {
            [urlLabel setText:alternateUrl];
            alternateUrl = [temp retain];
        }
    }
    [super touchesBegan:touches withEvent:event];
}

- (void)dealloc {
    [super dealloc];
}

static void *handle_client_2(void *param) {
    handle_client((int) (intptr_t) param);
    return NULL;
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
    
    int optval;
    optval = 1;
    setsockopt(ssock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    
    bzero(&sa, sizeof(sa));
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
        fd_set readset;
        FD_ZERO(&readset);
        FD_SET(ssock, &readset);
        FD_SET(pype[0], &readset);
        int nfds = (ssock > pype[0] ? ssock : pype[0]) + 1;
        nfds = select(nfds, &readset, NULL, NULL, NULL);
        if (nfds == -1) {
            err = errno;
            errprintf("Error in select() while waiting for connection: %s (%d)\n", strerror(err), err);
            goto done;
        }
        if (FD_ISSET(pype[0], &readset))
            goto done;
        if (!FD_ISSET(ssock, &readset))
            continue;
        unsigned int n = sizeof(ca);
        char cname[256];
        csock = accept(ssock, (struct sockaddr *) &ca, &n);
        if (csock == -1) {
            err = errno;
            errprintf("Could not accept connection from client: %s (%d)\n", strerror(err), err);
            goto done;
        }
        inet_ntop(AF_INET, &ca.sin_addr, cname, sizeof(cname));
        errprintf("Accepted connection from %s\n", cname);
        pthread_t thread;
        pthread_create(&thread, NULL, handle_client_2, (void *) (intptr_t) csock);
        pthread_detach(thread);
    }
    
done:
    port = 0;
    if (ssock != -1)
        close(ssock);
    close(pype[0]);
    close(pype[1]);
    [instance performSelectorOnMainThread:@selector(displayHostAndPort) withObject:nil waitUntilDone:NO];
    [pool release];
}

- (void) appendToLog:(NSString *) text {
    if (text == nil || [text length] == 0)
        return;
    NSString *temp = [logView text];
    if (temp == nil)
        temp = @"";
    [logView setText:[temp stringByAppendingString:text]];
    NSRange r;
    r.location = [[logView text] length];
    r.length = 0;
    [logView scrollRangeToVisible:r];
}

void errprintf(const char *fmt, ...) {
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
    
    va_list ap;
    char text[1024];
    va_start(ap, fmt);
    vsprintf(text, fmt, ap);
    [instance performSelectorOnMainThread:@selector(appendToLog:) withObject:[NSString stringWithCString:text encoding:NSUTF8StringEncoding] waitUntilDone:NO];
    va_end(ap);
    
    [pool release];
}

// C version of getVersion, needed by the HTTP Server

const char *get_version() {
    return [Free42AppDelegate getVersion];
}

char *make_temp_file() {
    static int fileno = 0;
    NSString *path = [NSString stringWithFormat:@"%@/tempzip.%d", NSTemporaryDirectory(), ++fileno];
    const char *cpath = [path UTF8String];
    size_t len = strlen(cpath);
    char *ret = (char *) malloc(len + 1);
    strcpy(ret, cpath);
    return ret;
}

@end
