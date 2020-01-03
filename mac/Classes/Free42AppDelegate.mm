/*****************************************************************************
 * Free42 -- an HP-42S calculator simulator
 * Copyright (C) 2004-2020  Thomas Okken
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

#import <AudioToolbox/AudioServices.h>
#import <IOKit/ps/IOPowerSources.h>
#import <sys/stat.h>
#import <sys/time.h>
#import "free42.h"
#import "shell.h"
#import "shell_skin.h"
#import "shell_spool.h"
#import "core_main.h"
#import "core_display.h"
#import "Free42AppDelegate.h"
#import "ProgramListDataSource.h"
#import "SkinListDataSource.h"
#import "CalcView.h"
#import "FileOpenPanel.h"
#import "FileSavePanel.h"
#import "PrintView.h"
#import "StatesWindow.h"


static Free42AppDelegate *instance = NULL;
static SystemSoundID soundIDs[11];

static bool launchingWithPrintoutVisible;
static bool scrollPrintoutToBottomInitially = true;
static bool loadSkinsWindowMapped = false;

state_type state;
char free42dirname[FILENAMELEN];

static int quit_flag = 0;
static int enqueued;
static int keep_running = 0;
static int we_want_cpu = 0;

static char statefilename[FILENAMELEN];
static char printfilename[FILENAMELEN];
static FILE *statefile = NULL;

static int ckey = 0;
static int skey;
static unsigned char *macro;
static bool macro_is_name;
static int mouse_key;
static unsigned short active_keycode = -1;
static int just_pressed_shift = 0;
static int keymap_length = 0;
static keymap_entry *keymap = NULL;

static bool timeout_active = false;
static int timeout_which;
static bool timeout3_active = false;
static bool repeater_active = false;

static int ann_updown = 0;
static int ann_shift = 0;
static int ann_print = 0;
static int ann_run = 0;
static int ann_battery = 0;
static int ann_g = 0;
static int ann_rad = 0;
static bool ann_print_timeout_active = false;

unsigned char *print_bitmap;
int printout_top;
int printout_bottom;
// Room for PRINT_LINES / 18 lines, plus two, plus one byte
#define PRINT_TEXT_SIZE 12551
unsigned char *print_text;
int print_text_top;
int print_text_bottom;
int print_text_pixel_height;

static FILE *print_txt = NULL;
static FILE *print_gif = NULL;
static char print_gif_name[FILENAMELEN];
static int gif_seq = -1;
static int gif_lines;

static void show_message(const char *title, const char *message);
static void read_key_map(const char *keymapfilename);
static void init_shell_state(int4 ver);
static int read_shell_state(int4 *ver);
static int write_shell_state();
static void shell_keydown();
static void shell_keyup();

static void txt_writer(const char *text, int length);
static void txt_newliner();
static void gif_seeker(int4 pos);
static void gif_writer(const char *text, int length);
static bool is_file(const char *name);

@implementation Free42AppDelegate

@synthesize mainWindow;
@synthesize calcView;
@synthesize printWindow;
@synthesize printView;
@synthesize preferencesWindow;
@synthesize prefsSingularMatrix;
@synthesize prefsMatrixOutOfRange;
@synthesize prefsAutoRepeat;
@synthesize prefsPrintText;
@synthesize prefsPrintTextFile;
@synthesize prefsPrintTextRaw;
@synthesize prefsPrintGIF;
@synthesize prefsPrintGIFFile;
@synthesize prefsPrintGIFMaxHeight;
@synthesize selectProgramsWindow;
@synthesize programListView;
@synthesize programListDataSource;
@synthesize aboutWindow;
@synthesize aboutVersion;
@synthesize aboutCopyright;
@synthesize loadSkinsWindow;
@synthesize loadSkinsURL;
@synthesize loadSkinButton;
@synthesize loadSkinsWebView;
@synthesize deleteSkinsWindow;
@synthesize skinListView;
@synthesize skinListDataSource;
@synthesize statesWindow;

static struct timeval runner_end_time;

- (void) startRunner {
    gettimeofday(&runner_end_time, NULL);
    runner_end_time.tv_usec += 10000; // run for up to 10 ms
    if (runner_end_time.tv_usec >= 1000000) {
        runner_end_time.tv_usec -= 1000000;
        runner_end_time.tv_sec++;
    }
    int dummy1, dummy2;
    keep_running = core_keydown(0, &dummy1, &dummy2);
    if (quit_flag)
        [self quit];
    else if (keep_running && !we_want_cpu)
        [self performSelector:@selector(startRunner) withObject:NULL afterDelay:0];
}

- (void)applicationWillFinishLaunching:(NSNotification *)aNotification {
    instance = self;
    
    /****************************/
    /***** Create sound IDs *****/
    /****************************/
    
    const char *sound_names[] = { "tone0", "tone1", "tone2", "tone3", "tone4", "tone5", "tone6", "tone7", "tone8", "tone9", "squeak" };
    for (int i = 0; i < 11; i++) {
        NSString *name = [NSString stringWithUTF8String:sound_names[i]];
        NSString *path = [[NSBundle mainBundle] pathForResource:name ofType:@"wav"];
        OSStatus status = AudioServicesCreateSystemSoundID((CFURLRef)[NSURL fileURLWithPath:path], &soundIDs[i]);
        if (status)
            NSLog(@"error loading sound:  %@", name);
    }
    
    
    /********************************************************************************/
    /***** Try to create the $HOME/Library/Application Support/Free42 directory *****/
    /********************************************************************************/
    
    int free42dir_exists = 0;
    char *home = getenv("HOME");
    struct stat st;
    char keymapfilename[FILENAMELEN];
    char oldfree42dirname[FILENAMELEN];
    
    snprintf(free42dirname, FILENAMELEN, "%s/Library/Application Support/Free42", home);
    if (stat(free42dirname, &st) == -1 || !S_ISDIR(st.st_mode)) {
        // Free42 directory does not exist. If $HOME/.free42 does exist, rename it and use
        // it instead (for compatibility with versions <= 1.4.54); else, create a new
        // Free42 directory.
        snprintf(oldfree42dirname, FILENAMELEN, "%s/.free42", home);
        if (stat(oldfree42dirname, &st) == 0 && S_ISDIR(st.st_mode))
            rename(oldfree42dirname, free42dirname);
        else
            mkdir(free42dirname, 0755);
        if (stat(free42dirname, &st) == 0 && S_ISDIR(st.st_mode))
            free42dir_exists = 1;
    } else
        free42dir_exists = 1;
    
    if (free42dir_exists) {
        snprintf(statefilename, FILENAMELEN, "%s/Library/Application Support/Free42/state", home);
        snprintf(printfilename, FILENAMELEN, "%s/Library/Application Support/Free42/print", home);
        snprintf(keymapfilename, FILENAMELEN, "%s/Library/Application Support/Free42/keymap.txt", home);
    } else {
        statefilename[0] = 0;
        printfilename[0] = 0;
        keymapfilename[0] = 0;
    }
    
    
    /****************************/
    /***** Read the key map *****/
    /****************************/
    
    read_key_map(keymapfilename);
    
    /******************************/
    /***** Read the print-out *****/
    /******************************/
    
    print_bitmap = (unsigned char *) malloc(PRINT_SIZE);
    print_text = (unsigned char *) malloc(PRINT_TEXT_SIZE);
    // TODO - handle memory allocation failure
    
    FILE *printfile = fopen(printfilename, "r");
    if (printfile != NULL) {
        int n = fread(&printout_bottom, 1, sizeof(int), printfile);
        if (n == sizeof(int)) {
            int bytes = printout_bottom * PRINT_BYTESPERLINE;
            n = fread(print_bitmap, 1, bytes, printfile);
            if (n == bytes) {
                n = fread(&print_text_bottom, 1, sizeof(int), printfile);
                int n2 = fread(&print_text_pixel_height, 1, sizeof(int), printfile);
                if (n == sizeof(int) && n2 == sizeof(int)) {
                    n = fread(print_text, 1, print_text_bottom, printfile);
                    if (n != print_text_bottom) {
                        print_text_bottom = 0;
                        print_text_pixel_height = 0;
                    }
                } else {
                    print_text_bottom = 0;
                    print_text_pixel_height = 0;
                }
            } else {
                printout_bottom = 0;
                print_text_bottom = 0;
                print_text_pixel_height = 0;
            }
        } else {
            printout_bottom = 0;
            print_text_bottom = 0;
            print_text_pixel_height = 0;
        }
        fclose(printfile);
    } else {
        printout_bottom = 0;
        print_text_bottom = 0;
        print_text_pixel_height = 0;
    }
    printout_top = 0;
    print_text_top = 0;
}

static void low_battery_checker(CFRunLoopTimerRef timer, void *info) {
    shell_low_battery();
}

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
    
    /***********************************************************/
    /***** Open the state file and read the shell settings *****/
    /***********************************************************/
    
    int4 version;
    int init_mode;
    char core_state_file_name[FILENAMELEN];
    int core_state_file_offset;
    if (statefilename[0] != 0)
        statefile = fopen(statefilename, "r");
    else
        statefile = NULL;
    if (statefile != NULL) {
        if (read_shell_state(&version)) {
            init_mode = 1;
        } else {
            init_shell_state(-1);
            init_mode = 2;
        }
    } else {
        init_shell_state(-1);
        init_mode = 0;
    }
    if (init_mode == 1) {
        if (version > 25) {
            snprintf(core_state_file_name, FILENAMELEN, "%s/%s.f42", free42dirname, state.coreName);
            core_state_file_offset = 0;
        } else {
            strcpy(core_state_file_name, statefilename);
            core_state_file_offset = ftell(statefile);
        }
        fclose(statefile);
    }  else {
        // The shell state was missing or corrupt, but there
        // may still be a valid core state...
        snprintf(core_state_file_name, FILENAMELEN, "%s/%s.f42", free42dirname, state.coreName);
        struct stat st;
        if (stat(core_state_file_name, &st) == 0) {
            // Core state "Untitled.f42" exists; let's try to read it
            core_state_file_offset = 0;
            init_mode = 1;
            version = 26;
        }
    }

#ifdef BCD_MATH
    [mainWindow setTitle:@"Free42 Decimal"];
#else
    [mainWindow setTitle:@"Free42 Binary"];
#endif
    long win_width, win_height;
    skin_load(&win_width, &win_height);
    NSSize sz;
    sz.width = win_width;
    sz.height = win_height;
    [mainWindow setContentSize:sz];
    
    if (state.mainWindowKnown) {
        NSPoint pt;
        pt.x = state.mainWindowX;
        pt.y = state.mainWindowY;
        [mainWindow setFrameOrigin:pt];
    }
    
    sz.width = 373;
    sz.height = state.printWindowKnown ? state.printWindowHeight : 600;
    [printWindow setContentSize:sz];
    [printView initialUpdate];
    
    if (state.printWindowKnown) {
        NSPoint pt;
        pt.x = state.printWindowX;
        pt.y = state.printWindowY;
        [printWindow setFrameOrigin:pt];
    }
    
    if (state.printWindowMapped) {
        launchingWithPrintoutVisible = true;
        [printWindow makeKeyAndOrderFront:self];
    } else {
        launchingWithPrintoutVisible = false;
        [mainWindow makeKeyAndOrderFront:self];
    }
    
    core_init(init_mode, version, core_state_file_name, core_state_file_offset);
    if (core_powercycle())
        [self startRunner];
    
    CFRunLoopTimerRef lowBatTimer = CFRunLoopTimerCreate(NULL, CFAbsoluteTimeGetCurrent(), 60, 0, 0, low_battery_checker, NULL);
    CFRunLoopAddTimer(CFRunLoopGetCurrent(), lowBatTimer, kCFRunLoopCommonModes);
}

- (void)applicationWillTerminate:(NSNotification *)aNotification {
    FILE *printfile;
    int n, length;
    
    printfile = fopen(printfilename, "w");
    if (printfile != NULL) {
        // Write bitmap
        length = printout_bottom - printout_top;
        if (length < 0)
            length += PRINT_LINES;
        n = fwrite(&length, 1, sizeof(int), printfile);
        if (n != sizeof(int))
            goto failed;
        if (printout_bottom >= printout_top) {
            n = fwrite(print_bitmap + PRINT_BYTESPERLINE * printout_top,
                       1, PRINT_BYTESPERLINE * length, printfile);
            if (n != PRINT_BYTESPERLINE * length)
                goto failed;
        } else {
            n = fwrite(print_bitmap + PRINT_BYTESPERLINE * printout_top,
                       1, PRINT_SIZE - PRINT_BYTESPERLINE * printout_top,
                       printfile);
            if (n != PRINT_SIZE - PRINT_BYTESPERLINE * printout_top)
                goto failed;
            n = fwrite(print_bitmap, 1,
                       PRINT_BYTESPERLINE * printout_bottom, printfile);
            if (n != PRINT_BYTESPERLINE * printout_bottom)
                goto failed;
        }
        // Write text
        length = print_text_bottom - print_text_top;
        if (length < 0)
            length += PRINT_TEXT_SIZE;
        n = fwrite(&length, 1, sizeof(int), printfile);
        if (n != sizeof(int))
            goto failed;
        n = fwrite(&print_text_pixel_height, 1, sizeof(int), printfile);
        if (n != sizeof(int))
            goto failed;
        if (print_text_bottom >= print_text_top) {
            n = fwrite(print_text + print_text_top, 1, length, printfile);
            if (n != length)
                goto failed;
        } else {
            n = fwrite(print_text + print_text_top, 1, PRINT_TEXT_SIZE - print_text_top, printfile);
            if (n != PRINT_TEXT_SIZE - print_text_top)
                goto failed;
            n = fwrite(print_text, 1, print_text_bottom, printfile);
            if (n != print_text_bottom)
                goto failed;
        }
        
        fclose(printfile);
        goto done;
        
    failed:
        fclose(printfile);
        remove(printfilename);
        
    done:
        ;
    }
    
    if (print_txt != NULL)
        fclose(print_txt);
    
    if (print_gif != NULL) {
        shell_finish_gif(gif_seeker, gif_writer);
        fclose(print_gif);
    }
    
    shell_spool_exit();
    
    state.mainWindowX = (int) mainWindow.frame.origin.x;
    state.mainWindowY = (int) mainWindow.frame.origin.y;
    state.mainWindowKnown = 1;
    state.printWindowX = (int) printWindow.frame.origin.x;
    state.printWindowY = (int) printWindow.frame.origin.y;
    state.printWindowHeight = (int) [[printWindow contentView] frame].size.height;
    state.printWindowKnown = 1;
    statefile = fopen(statefilename, "w");
    if (statefile != NULL) {
        write_shell_state();
        fclose(statefile);
    }
    char corefilename[FILENAMELEN];
    snprintf(corefilename, FILENAMELEN, "%s/%s.f42", free42dirname, state.coreName);
    core_save_state(corefilename);
    core_cleanup();
}

- (void)windowWillClose:(NSNotification *)notification {
    NSWindow *window = [notification object];
    if (window == aboutWindow || window == preferencesWindow || window == selectProgramsWindow || window == deleteSkinsWindow || window == statesWindow) {
        [NSApp stopModal];
        if (window == preferencesWindow)
            [instance getPreferences];
    } else if (window == mainWindow)
        [NSApp terminate:nil];
    else if (window == printWindow)
        state.printWindowMapped = 0;
    else if (window == loadSkinsWindow)
        loadSkinsWindowMapped = false;
}

- (void)windowDidBecomeKey:(NSNotification *)notification {
    NSWindow *window = [notification object];
    if (window == printWindow) {
        if (launchingWithPrintoutVisible) {
            launchingWithPrintoutVisible = false;
            [mainWindow performSelectorOnMainThread:@selector(makeKeyAndOrderFront:) withObject:self waitUntilDone:NO];
        }
        if (scrollPrintoutToBottomInitially) {
            scrollPrintoutToBottomInitially = false;
            [printView performSelectorOnMainThread:@selector(scrollToBottom) withObject:nil waitUntilDone:NO];
        }
    }
}

- (IBAction) showAbout:(id)sender {
    const char *version = [Free42AppDelegate getVersion];
    [aboutVersion setStringValue:[NSString stringWithFormat:@"Free42 %s", version]];
    [aboutCopyright setStringValue:@"© 2004-2020 Thomas Okken"];
    [NSApp runModalForWindow:aboutWindow];
}

- (IBAction) showPreferences:(id)sender {
    [prefsSingularMatrix setState:core_settings.matrix_singularmatrix];
    [prefsMatrixOutOfRange setState:core_settings.matrix_outofrange];
    [prefsAutoRepeat setState:core_settings.auto_repeat];
    [prefsPrintText setState:state.printerToTxtFile];
    [prefsPrintTextFile setStringValue:[NSString stringWithUTF8String:state.printerTxtFileName]];
    [prefsPrintGIF setState:state.printerToGifFile];
    [prefsPrintGIFFile setStringValue:[NSString stringWithUTF8String:state.printerGifFileName]];
    [prefsPrintGIFMaxHeight setStringValue:[NSString stringWithFormat:@"%d", state.printerGifMaxLength]];
    [NSApp runModalForWindow:preferencesWindow];
}

- (void) getPreferences {
    core_settings.matrix_singularmatrix = [prefsSingularMatrix state];
    core_settings.matrix_outofrange = [prefsMatrixOutOfRange state];
    core_settings.auto_repeat = [prefsAutoRepeat state];
    state.printerToTxtFile = [prefsPrintText state];
    char buf[FILENAMELEN];
    [[prefsPrintTextFile stringValue] getCString:buf maxLength:FILENAMELEN encoding:NSUTF8StringEncoding];
    int len = strlen(buf);
    if (len > 0 && (len < 4 || strcasecmp(buf + len - 4, ".txt") != 0))
        strcat(buf, ".txt");
    if (print_txt != NULL && (!state.printerToTxtFile || strcasecmp(state.printerTxtFileName, buf) != 0)) {
        fclose(print_txt);
        print_txt = NULL;
    }
    strcpy(state.printerTxtFileName, buf);
    state.printerToGifFile = [prefsPrintGIF state];
    [[prefsPrintGIFFile stringValue] getCString:buf maxLength:FILENAMELEN encoding:NSUTF8StringEncoding];
    len = strlen(buf);
    if (len > 0 && (len < 4 || strcasecmp(buf + len - 4, ".gif") != 0))
        strcat(buf, ".gif");
    if (print_gif != NULL && (!state.printerToGifFile || strcasecmp(state.printerGifFileName, buf) != 0)) {
        shell_finish_gif(gif_seeker, gif_writer);
        fclose(print_gif);
        print_gif = NULL;
        gif_seq = -1;
    }
    strcpy(state.printerGifFileName, buf);
    [[prefsPrintGIFMaxHeight stringValue] getCString:buf maxLength:50 encoding:NSUTF8StringEncoding];
    if (sscanf(buf, "%d", &state.printerGifMaxLength) == 1) {
        if (state.printerGifMaxLength < 16)
            state.printerGifMaxLength = 16;
        else if (state.printerGifMaxLength > 32767)
            state.printerGifMaxLength = 32767;
    } else
        state.printerGifMaxLength = 256;
}

- (IBAction) browsePrintTextFile:(id)sender {
    FileSavePanel *saveDlg = [FileSavePanel panelWithTitle:@"Select Text File Name" types:@"Text;txt;All Files;*" path:[prefsPrintTextFile stringValue]];
    if ([saveDlg runModal] == NSOKButton)
        [prefsPrintTextFile setStringValue:[saveDlg path]];
}

- (IBAction) browsePrintGIFFile:(id)sender {
    FileSavePanel *saveDlg = [FileSavePanel panelWithTitle:@"Select GIF File Name" types:@"GIF;gif;All Files;*" path:[prefsPrintGIFFile stringValue]];
    if ([saveDlg runModal] == NSOKButton)
        [prefsPrintGIFFile setStringValue:[saveDlg path]];
}

- (IBAction) states:(id)sender {
    [statesWindow reset];
    [NSApp runModalForWindow:statesWindow];
}

- (IBAction) showPrintOut:(id)sender {
    [printWindow makeKeyAndOrderFront:self];
    state.printWindowMapped = 1;
}

- (IBAction) clearPrintOut:(id)sender {
    printout_top = printout_bottom = 0;
    print_text_top = print_text_bottom = print_text_pixel_height = 0;
    NSSize s;
    s.width = 358;
    s.height = 0;
    [printView setFrameSize:s];
}

- (IBAction) importPrograms:(id)sender {
    FileOpenPanel *openDlg = [FileOpenPanel panelWithTitle:@"Import Programs" types:@"Program Files;raw;All Files;*"];
    if ([openDlg runModal] == NSOKButton) {
        NSArray* paths = [openDlg paths];
        for (int i = 0; i < [paths count]; i++) {
            NSString* fileName = [paths objectAtIndex:i];
            char cFileName[1024];
            [fileName getCString:cFileName maxLength:1024 encoding:NSUTF8StringEncoding];
            core_import_programs(0, cFileName);
            redisplay();
        }
    }
}

- (IBAction) exportPrograms:(id)sender {
    char *buf = core_list_programs();
    [programListDataSource setProgramNames:buf];
    free(buf);
    [programListView reloadData];
    [NSApp runModalForWindow:selectProgramsWindow];
}

- (IBAction) exportProgramsCancel:(id)sender {
    [NSApp stopModal];
    [selectProgramsWindow orderOut:self];
}

- (IBAction) exportProgramsOK:(id)sender {
    [NSApp stopModal];
    [selectProgramsWindow orderOut:self];
    bool *selection = [programListDataSource getSelection];
    int count = [programListDataSource numberOfRowsInTableView:nil];
    int firstSelected = -1;
    for (int i = 0; i < count; i++)
        if (selection[i]) {
            firstSelected = i;
            break;
        }
    if (firstSelected == -1)
        return;
    NSString *name = [programListDataSource getItemAtIndex:firstSelected];
    if ([name characterAtIndex:0] != '"')
        name = @"Untitled";
    else {
        NSRange range = [name rangeOfString:@"\"" options:0 range:NSMakeRange(1, [name length] - 1)];
        if (range.location == NSNotFound)
            name = @"Untitled";
        else
            name = [name substringWithRange:NSMakeRange(1, range.location - 1)];
    }
    FileSavePanel *saveDlg = [FileSavePanel panelWithTitle:@"Export Programs" types:@"Program Files;raw;All Files;*" path:name];
    if ([saveDlg runModal] == NSOKButton) {
        NSString *fileName = [saveDlg path];
        char cFileName[1024];
        [fileName getCString:cFileName maxLength:1024 encoding:NSUTF8StringEncoding];
        int *indexes = (int *) malloc(count * sizeof(int));
        int selectionSize = 0;
        for (int i = 0; i < count; i++)
            if (selection[i])
                indexes[selectionSize++] = i;
        core_export_programs(selectionSize, indexes, cFileName);
        free(indexes);
    }
}

- (IBAction) doCopy:(id)sender {
    NSPasteboard *pb = [NSPasteboard generalPasteboard];
    NSArray *types = [NSArray arrayWithObjects: NSStringPboardType, nil];
    [pb declareTypes:types owner:self];
    NSString *txt;
    if ([loadSkinsWindow isKeyWindow]) {
        txt = [loadSkinsURL stringValue];
    } else {
        char *buf = core_copy();
        txt = [NSString stringWithUTF8String:buf];
        free(buf);
    }
    [pb setString:txt forType:NSStringPboardType];
}

- (IBAction) doPaste:(id)sender {
    NSPasteboard *pb = [NSPasteboard generalPasteboard];
    NSArray *types = [NSArray arrayWithObjects: NSStringPboardType, nil];
    NSString *bestType = [pb availableTypeFromArray:types];
    if (bestType != nil) {
        NSString *txt = [pb stringForType:NSStringPboardType];
        if ([loadSkinsWindow isKeyWindow]) {
            [loadSkinsURL setStringValue:txt];
        } else {
            const char *buf = [txt UTF8String];
            core_paste(buf);
        }
    }
}

static char *tb;
static int tblen, tbcap;

static void tbwriter(const char *text, int length) {
    if (tblen + length > tbcap) {
        tbcap += length + 8192;
        tb = (char *) realloc(tb, tbcap);
    }
    if (tb != NULL) {
        memcpy(tb + tblen, text, length);
        tblen += length;
    }
}

static void tbnewliner() {
    tbwriter("\n", 1);
}

static void tbnonewliner() {
    // No-op
}

- (IBAction) doCopyPrintOutAsText:(id)sender {
    NSPasteboard *pb = [NSPasteboard generalPasteboard];
    NSArray *types = [NSArray arrayWithObjects: NSStringPboardType, nil];
    [pb declareTypes:types owner:self];

    tb = NULL;
    tblen = tbcap = 0;

    int len = print_text_bottom - print_text_top;
    if (len < 0)
        len += PRINT_TEXT_SIZE;
    // Calculate effective top, since printout_top can point
    // at a truncated line, and we want to skip those when
    // copying
    int top = printout_bottom - 2 * print_text_pixel_height;
    if (top < 0)
        top += PRINT_LINES;
    int p = print_text_top;
    int pixel_v = 0;
    while (len > 0) {
        int z = print_text[p++];
        if (z == 255) {
            char buf[34];
            for (int v = 0; v < 16; v += 2) {
                for (int vv = 0; vv < 2; vv++) {
                    int V = top + (pixel_v + v + vv) * 2;
                    if (V >= PRINT_LINES)
                        V -= PRINT_LINES;
                    for (int h = 0; h < 17; h++) {
                        unsigned char a = print_bitmap[V * PRINT_BYTESPERLINE + 2 * h + 1];
                        unsigned char b = print_bitmap[V * PRINT_BYTESPERLINE + 2 * h];
                        buf[vv * 17 + h] = (a & 128) | ((a & 32) << 1) | ((a & 8) << 2) | ((a & 2) << 3) | ((b & 128) >> 4) | ((b & 32) >> 3) | ((b & 8) >> 2) | ((b & 2) >> 1);
                    }
                }
                shell_spool_bitmap_to_txt(buf, 17, 0, 0, 131, 2, tbwriter, tbnewliner);
            }
            pixel_v += 16;
        } else {
            if (p + z < PRINT_TEXT_SIZE) {
                shell_spool_txt((const char *) (print_text + p), z, tbwriter, tbnewliner);
                p += z;
            } else {
                int d = PRINT_TEXT_SIZE - p;
                shell_spool_txt((const char *) (print_text + p), d, tbwriter, tbnonewliner);
                shell_spool_txt((const char *) print_text, z - d, tbwriter, tbnewliner);
                p = z - d;
            }
            len -= z;
            pixel_v += 9;
        }
        len--;
    }
    tbwriter("\0", 1);

    NSString *txt;
    if (tb == NULL) {
        txt = @"";
    } else {
        txt = [NSString stringWithUTF8String:tb];
        free(tb);
    }

    [pb setString:txt forType:NSStringPboardType];
}

- (IBAction) doCopyPrintOutAsImage:(id)sender {
    NSPasteboard *pb = [NSPasteboard generalPasteboard];
    NSArray *types = [NSArray arrayWithObjects: NSTIFFPboardType, nil];
    [pb declareTypes:types owner:self];
    
    int height = printout_bottom - printout_top;
    if (height < 0)
        height += PRINT_LINES;
    
    NSBitmapImageRep *img = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes:NULL pixelsWide:358 pixelsHigh:(height == 0 ? 1 : height) bitsPerSample:1 samplesPerPixel:1 hasAlpha:NO isPlanar:NO colorSpaceName:NSCalibratedWhiteColorSpace bytesPerRow:45 bitsPerPixel:1];
    unsigned char *data = [img bitmapData];
    if (height == 0) {
        memset(data, 255, 36);
    } else {
        for (int v = 0; v < height; v++) {
            int vv = printout_top + v;
            if (vv >= PRINT_LINES)
                vv -= PRINT_LINES;
            unsigned char *src = print_bitmap + vv * PRINT_BYTESPERLINE;
            *data++ = 255;
            *data++ = 255;
            *data++ = 255;
            *data++ = 255;
            unsigned char pc = 0;
            for (int h = 0; h <= 36; h++) {
                unsigned char c = h == 36 ? 0 : src[h];
                *data++ = (((pc & 16) << 3) | ((pc & 32) << 1) | ((pc & 64) >> 1) | ((pc & 128) >> 3) | ((c & 1) << 3) | ((c & 2) << 1) | ((c & 4) >> 1) | ((c & 8) >> 3)) ^ 255;
                pc = c;
            }
            *data++ = 255;
            *data++ = 255;
            *data++ = 255;
            *data++ = 255;
        }
    }
    
    NSData *tiff = [img TIFFRepresentation];
    [pb setData:tiff forType:NSTIFFPboardType];
}

- (IBAction) paperAdvance:(id)sender {
    static const char *bits = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
    shell_print("", 0, bits, 18, 0, 0, 143, 9);
}

- (IBAction) loadSkins:(id)sender {
    if (!loadSkinsWindowMapped) {
        NSString *url = @"https://thomasokken.com/free42/skins/";
        [loadSkinsURL setStringValue:url];
        [loadSkinsWebView setMainFrameURL:url];
        loadSkinsWindowMapped = true;
    }
    [loadSkinsWindow makeKeyAndOrderFront:self];
    [loadSkinsURL becomeFirstResponder];
}

- (IBAction) loadSkinsGo:(id)sender {
    NSString *url = [loadSkinsURL stringValue];
    [loadSkinsWebView setMainFrameURL:url];
}

- (IBAction) loadSkinsBack:(id)sender {
    [loadSkinsWebView goBack];
}

- (IBAction) loadSkinsForward:(id)sender {
    [loadSkinsWebView goForward];
}

- (void)webView:(WebView *)sender didStartProvisionalLoadForFrame:(WebFrame *)frame {
    [loadSkinButton setEnabled:NO];
    [loadSkinButton setTitle:@"..."];
}

- (void)webView:(WebView *)sender didFinishLoadForFrame:(WebFrame *)frame {
    NSString *url = [loadSkinsWebView mainFrameURL];
    [loadSkinsURL setStringValue:url];
    [loadSkinButton setTitle:@"Load"];
    [loadSkinButton setEnabled:[Free42AppDelegate skinUrlPair:url] != nil];
}

- (BOOL) tryLoad:(NSString *)url asFile:(NSString *)name {
    NSURLRequest *req = [NSURLRequest requestWithURL:[NSURL URLWithString:url] cachePolicy:NSURLRequestReloadIgnoringCacheData timeoutInterval:60];
    NSURLResponse *resp;
    NSError *err;
    NSData *data = [NSURLConnection sendSynchronousRequest:req returningResponse:&resp error:&err];
    if (data == nil)
        return NO;
    NSString *fname = [NSString stringWithFormat:@"%s/%@", free42dirname, name];
    return [data writeToFile:fname atomically:NO];
}

- (IBAction) loadSkinsLoad:(id)sender {
    NSString *url = [loadSkinsURL stringValue];
    NSArray *urls = [Free42AppDelegate skinUrlPair:url];
    if (urls == nil) {
        show_message("Error", "Invalid Skin URL");
        return;
    }
    if ([self tryLoad:[urls objectAtIndex:0] asFile:@"_temp_gif_"]
        && [self tryLoad:[urls objectAtIndex:1] asFile:@"_temp_layout_"]) {
        char buf1[FILENAMELEN], buf2[FILENAMELEN];
        snprintf(buf1, FILENAMELEN, "%s/_temp_gif_", free42dirname);
        snprintf(buf2, FILENAMELEN, "%s/%s.gif", free42dirname, [[urls objectAtIndex:2] UTF8String]);
        rename(buf1, buf2);
        snprintf(buf1, FILENAMELEN, "%s/_temp_layout_", free42dirname);
        snprintf(buf2, FILENAMELEN, "%s/%s.layout", free42dirname, [[urls objectAtIndex:2] UTF8String]);
        rename(buf1, buf2);
        show_message("Message", "Skin Loaded");
    } else {
        char buf[FILENAMELEN];
        snprintf(buf, FILENAMELEN, "%s/_temp_gif_", free42dirname);
        remove(buf);
        snprintf(buf, FILENAMELEN, "%s/_temp_layout_", free42dirname);
        remove(buf);
        show_message("Error", "Loading Skin Failed");
    }
}

+ (NSArray *)skinUrlPair:(NSString *)url {
    NSURLComponents *u = [NSURLComponents componentsWithString:url];
    NSString *path = [u path];
    if (path == nil)
        return nil;
    NSString *gifUrl = nil, *layoutUrl = nil;
    if ([path hasSuffix:@".gif"]) {
        gifUrl = url;
        NSRange r = NSMakeRange(0, [path length] - 3);
        NSString *layoutPath = [[path substringWithRange:r] stringByAppendingString:@"layout"];
        [u setPath:layoutPath];
        layoutUrl = [u string];
    } else if ([path hasSuffix:@".layout"]) {
        layoutUrl = url;
        NSRange r = NSMakeRange(0, [path length] - 6);
        NSString *gifPath = [[path substringWithRange:r] stringByAppendingString:@"gif"];
        [u setPath:gifPath];
        gifUrl = [u string];
    } else {
        return nil;
    }
    NSString *baseName = [gifUrl lastPathComponent];
    return [NSArray arrayWithObjects:gifUrl, layoutUrl, [baseName substringToIndex:[baseName length] - 4], nil];
}

- (IBAction) deleteSkins:(id)sender {
    [skinListDataSource loadSkinNames];
    [skinListView reloadData];
    [NSApp runModalForWindow:deleteSkinsWindow];
}

- (IBAction) deleteSkinsCancel:(id)sender {
    [NSApp stopModal];
    [deleteSkinsWindow orderOut:self];
}

- (IBAction) deleteSkinsOK:(id)sender {
    [NSApp stopModal];
    [deleteSkinsWindow orderOut:self];
    bool *selection = [skinListDataSource getSelection];
    NSString **names = [skinListDataSource getNames];
    int count = [skinListDataSource numberOfRowsInTableView:nil];
    for (int i = 0; i < count; i++)
        if (selection[i]) {
            const char *fname = [[NSString stringWithFormat:@"%s/%@.gif", free42dirname, names[i]] UTF8String];
            remove(fname);
            fname = [[NSString stringWithFormat:@"%s/%@.layout", free42dirname, names[i]] UTF8String];
            remove(fname);
        }
}

static char version[32] = "";

+ (const char *) getVersion {
    if (version[0] == 0) {
        NSString *appVersion = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"CFBundleVersion"];
        strcpy(version, [appVersion UTF8String]);
        // Version string consists of up to four dot-separated numbers.
        // If there are four, change the last ".nn" to a letter.
        int pos, num;
        if (sscanf(version, "%*d.%*d.%*d.%n%d", &pos, &num) == 1) {
            version[pos - 1] = 'a' + num - 1;
            version[pos] = 0;
        }
    }
    return version;
}

+ (void) showMessage:(NSString *)message withTitle:(NSString *)title {
    NSAlert *alert = [[NSAlert alloc] init];
    [alert addButtonWithTitle:@"OK"];
    [alert setMessageText:title];
    [alert setInformativeText:message];
    [alert setAlertStyle:NSCriticalAlertStyle];
    [alert runModal];
}

+ (void) showCMessage:(const char *)message withTitle:(const char *)title {
    [Free42AppDelegate showMessage:[NSString stringWithUTF8String:message] withTitle:[NSString stringWithUTF8String:title]];
}

- (IBAction) menuNeedsUpdate:(NSMenu *)menu {
    skin_menu_update(menu);
}

- (void) selectSkin:(id)sender {
    NSMenuItem *item = (NSMenuItem *) sender;
    NSString *name = [item title];
    [name getCString:state.skinName maxLength:FILENAMELEN encoding:NSUTF8StringEncoding];
    long w, h;
    skin_load(&w, &h);
    core_repaint_display();
    NSSize sz;
    sz.width = w;
    sz.height = h;
    NSRect frame = [mainWindow frame];
    NSPoint p;
    p.x = frame.origin.x;
    p.y = frame.origin.y + frame.size.height;
    [mainWindow setContentSize:sz];
    [mainWindow setFrameTopLeftPoint:p];
    [calcView setNeedsDisplayInRect:CGRectMake(0, 0, w, h)];
}

+ (void) loadState:(const char *)name {
    if (strcmp(name, state.coreName) != 0) {
        char corefilename[FILENAMELEN];
        snprintf(corefilename, FILENAMELEN, "%s/%s.f42", free42dirname, state.coreName);
        core_save_state(corefilename);
    }
    core_cleanup();
    strcpy(state.coreName, name);
    char corefilename[FILENAMELEN];
    snprintf(corefilename, FILENAMELEN, "%s/%s.f42", free42dirname, state.coreName);
    core_init(1, 26, corefilename, 0);
    if (core_powercycle())
        [instance startRunner];
}

- (void) quit {
    [NSApp terminate:nil];
}

- (void) setTimeout:(int) which {
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(timeout_callback) object:NULL];
    timeout_which = which;
    timeout_active = true;
    [self performSelector:@selector(timeout_callback) withObject:NULL afterDelay:(which == 1 ? 0.25 : 1.75)];
}

- (void) cancelTimeout {
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(timeout_callback) object:NULL];
    timeout_active = false;
}

- (void) timeout_callback {
    timeout_active = false;
    if (ckey != 0) {
        if (timeout_which == 1) {
            core_keytimeout1();
            [self setTimeout:2];
        } else if (timeout_which == 2) {
            core_keytimeout2();
        }
    }
}

- (void) cancelTimeout3 {
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(timeout3_callback) object:NULL];
    timeout3_active = false;
}

- (void) setTimeout3: (int) delay {
    [self cancelTimeout3];
    [self performSelector:@selector(timeout3_callback) withObject:NULL afterDelay:(delay / 1000.0)];
    timeout3_active = true;
}

- (void) timeout3_callback {
    timeout3_active = false;
    keep_running = core_timeout3(1);
    if (keep_running)
        [self startRunner];
}

- (void) setRepeater: (int) delay {
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(repeater_callback) object:NULL];
    [self performSelector:@selector(repeater_callback) withObject:NULL afterDelay:(delay / 1000.0)];
    repeater_active = true;
}

- (void) cancelRepeater {
    [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(repeater_callback) object:NULL];
    repeater_active = false;
}

- (void) repeater_callback {
    int repeat = core_repeat();
    if (repeat != 0)
        [self setRepeater:(repeat == 1 ? 200 : 100)];
    else
        [self setTimeout:1];
}

- (void) turn_off_print_ann {
    ann_print = 0;
    skin_update_annunciator(3, 0);
    ann_print_timeout_active = FALSE;
}

- (void) print_ann_helper:(int)prt {
    if (ann_print_timeout_active) {
        [NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(turn_off_print_ann) object:NULL];
        ann_print_timeout_active = FALSE;
    }
    if (ann_print != prt) {
        if (prt) {
            ann_print = 1;
            skin_update_annunciator(3, ann_print);
        } else {
            [self performSelector:@selector(turn_off_print_ann) withObject:NULL afterDelay:1];
            ann_print_timeout_active = TRUE;
        }
    }
}

@end

static void shell_keydown() {
    int repeat;
    if (skey == -1)
        skey = skin_find_skey(ckey);
    skin_set_pressed_key(skey);
    if (timeout3_active && (macro != NULL || ckey != 28 /* KEY_SHIFT */)) {
        [instance cancelTimeout3];
        core_timeout3(0);
    }
    
    // We temporarily set we_want_cpu to 'true', to force the calls
    // to core_keydown() in this function to return quickly. This is
    // necessary since this function runs on the main thread, and we
    // can't peek ahead in the event queue while core_keydown() is
    // hogging the CPU on the main thread. (The lack of something like
    // EventAvail is an annoying omission of the iPhone API.)
    
    if (macro != NULL) {
        if (macro_is_name) {
            we_want_cpu = 1;
            keep_running = core_keydown_command((const char *) macro, &enqueued, &repeat);
            we_want_cpu = 0;
        } else {
            if (*macro == 0) {
                squeak();
                return;
            }
            bool one_key_macro = macro[1] == 0 || (macro[2] == 0 && macro[0] == 28);
            if (!one_key_macro) {
                skin_display_set_enabled(false);
            }
            while (*macro != 0) {
                we_want_cpu = 1;
                keep_running = core_keydown(*macro++, &enqueued, &repeat);
                we_want_cpu = 0;
                if (*macro != 0 && !enqueued)
                    core_keyup();
            }
            if (!one_key_macro) {
                skin_display_set_enabled(true);
                skin_repaint_display();
                /*
                skin_repaint_annunciator(1, ann_updown);
                skin_repaint_annunciator(2, ann_shift);
                skin_repaint_annunciator(3, ann_print);
                skin_repaint_annunciator(4, ann_run);
                skin_repaint_annunciator(5, ann_battery);
                skin_repaint_annunciator(6, ann_g);
                skin_repaint_annunciator(7, ann_rad);
                */
                repeat = 0;
            }
        }
    } else {
        we_want_cpu = 1;
        keep_running = core_keydown(ckey, &enqueued, &repeat);
        we_want_cpu = 0;
    }
    
    if (quit_flag)
        [NSApp terminate:nil];
    else if (keep_running)
        [instance startRunner];
    else {
        [instance cancelTimeout];
        [instance cancelRepeater];
        if (repeat != 0)
            [instance setRepeater:(repeat == 1 ? 1000 : 500)];
        else if (!enqueued)
            [instance setTimeout:1];
    }
}

static void shell_keyup() {
    skin_set_pressed_key(-1);
    ckey = 0;
    skey = -1;
    [instance cancelTimeout];
    [instance cancelRepeater];
    if (!enqueued) {
        keep_running = core_keyup();
        if (quit_flag)
            [NSApp terminate:nil];
        else if (keep_running)
            [instance startRunner];
    } else if (keep_running) {
        [instance startRunner];
    }
}

void calc_mousedown(int x, int y) {
    if (ckey == 0) {
        skin_find_key(x, y, ann_shift != 0, &skey, &ckey);
        if (ckey != 0) {
            macro = skin_find_macro(ckey, &macro_is_name);
            shell_keydown();
            mouse_key = 1;
        }
    }
}

void calc_mouseup() {
    if (ckey != 0 && mouse_key)
        shell_keyup();
}

void calc_keydown(NSString *characters, NSUInteger flags, unsigned short keycode) {
    if (ckey != 0 && mouse_key)
        return;
    
    int len = [characters length];
    if (len == 0)
        return;
    unsigned short c = [characters characterAtIndex:0];
    
    bool printable = len == 1 && c >= 32 && c <= 126;
    just_pressed_shift = 0;
    
    bool ctrl = (flags & NSControlKeyMask) != 0;
    bool alt = (flags & NSAlternateKeyMask) != 0;
    bool shift = (flags & NSShiftKeyMask) != 0;
    bool cshift = ann_shift != 0;
    
    if (ckey != 0) {
        shell_keyup();
        active_keycode = -1;
    }
    
    bool exact;
    unsigned char *key_macro = skin_keymap_lookup(c, printable,
                                                  ctrl, alt, shift, cshift, &exact);
    if (key_macro == NULL || !exact) {
        for (int i = 0; i < keymap_length; i++) {
            keymap_entry *entry = keymap + i;
            if (ctrl == entry->ctrl
                && alt == entry->alt
                && (printable || shift == entry->shift)
                && c == entry->keychar) {
                if (cshift == entry->cshift) {
                    key_macro = entry->macro;
                    break;
                } else {
                    if (cshift && key_macro == NULL)
                        key_macro = entry->macro;
                }
            }
        }
    }

    if (key_macro == NULL || (key_macro[0] != 36 || key_macro[1] != 0)
            && (key_macro[0] != 28 || key_macro[1] != 36 || key_macro[2] != 0)) {
        // The test above is to make sure that whatever mapping is in
        // effect for R/S will never be overridden by the special cases
        // for the ALPHA and A..F menus.
        if (!ctrl && !alt) {
            if (printable && core_alpha_menu()) {
                if (c >= 'a' && c <= 'z')
                    c = c + 'A' - 'a';
                else if (c >= 'A' && c <= 'Z')
                    c = c + 'a' - 'A';
                ckey = 1024 + c;
                skey = -1;
                macro = NULL;
                shell_keydown();
                mouse_key = 0;
                active_keycode = keycode;
                return;
            } else if (core_hex_menu() && ((c >= 'a' && c <= 'f')
                                        || (c >= 'A' && c <= 'F'))) {
                if (c >= 'a' && c <= 'f')
                    ckey = c - 'a' + 1;
                else
                    ckey = c - 'A' + 1;
                skey = -1;
                macro = NULL;
                shell_keydown();
                mouse_key = 0;
                active_keycode = keycode;
                return;
            }
        }
    }
    
    if (key_macro != NULL) {
        // A keymap entry is a sequence of zero or more calculator
        // keystrokes (1..37) and/or macros (38..255). We expand
        // macros here before invoking shell_keydown().
        // If the keymap entry is one key, or two keys with the
        // first being 'shift', we highlight the key in question
        // by setting ckey; otherwise, we set ckey to -10, which
        // means no skin key will be highlighted.
        ckey = -10;
        skey = -1;
        if (key_macro[0] != 0)
            if (key_macro[1] == 0)
                ckey = key_macro[0];
            else if (key_macro[2] == 0 && key_macro[0] == 28)
                ckey = key_macro[1];
        bool needs_expansion = false;
        for (int j = 0; key_macro[j] != 0; j++)
            if (key_macro[j] > 37) {
                needs_expansion = true;
                break;
            }
        if (needs_expansion) {
            static unsigned char macrobuf[1024];
            int p = 0;
            for (int j = 0; key_macro[j] != 0 && p < 1023; j++) {
                int c = key_macro[j];
                if (c <= 37)
                    macrobuf[p++] = c;
                else {
                    unsigned char *m = skin_find_macro(c, &macro_is_name);
                    if (m != NULL)
                        while (*m != 0 && p < 1023)
                            macrobuf[p++] = *m++;
                }
            }
            macrobuf[p] = 0;
            macro = macrobuf;
        } else {
            macro = key_macro;
            macro_is_name = false;
        }
        shell_keydown();
        mouse_key = 0;
        active_keycode = keycode;
    }
}

void calc_keyup(NSString *characters, NSUInteger flags, unsigned short keycode) {
    if (ckey != 0) {
        if (!mouse_key && keycode == active_keycode) {
            shell_keyup();
            active_keycode = -1;
        }
    }
}

void calc_keymodifierschanged(NSUInteger flags) {
    static bool shift_was_down = false;
    bool shift_is_down = (flags & NSShiftKeyMask) != 0;
    if (shift_is_down == shift_was_down)
        return;
    shift_was_down = shift_is_down;
    if (shift_is_down) {
        // Shift pressed
        just_pressed_shift = 1;
    } else {
        // Shift released
        if (ckey == 0 && just_pressed_shift) {
            ckey = 28;
            skey = -1;
            macro = NULL;
            shell_keydown();
            shell_keyup();
        }
    }
}

static void show_message(const char *title, const char *message) {
    [Free42AppDelegate showCMessage:message withTitle:title];
}

void shell_message(const char *message) {
    [Free42AppDelegate showCMessage:message withTitle:"Core"];
}

const char *shell_platform() {
    static char p[16];
    strncpy(p, [Free42AppDelegate getVersion], 16);
    strncat(p, " MacOS", 16);
    p[15] = 0;
    return p;
}

int8 shell_random_seed() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000LL + tv.tv_usec / 1000;
}

void shell_beeper(int frequency, int duration) {
    const int cutoff_freqs[] = { 164, 220, 243, 275, 293, 324, 366, 418, 438, 550 };
    for (int i = 0; i < 10; i++) {
        if (frequency <= cutoff_freqs[i]) {
            AudioServicesPlaySystemSound(soundIDs[i]);
            shell_delay(250);
            return;
        }
    }
    AudioServicesPlaySystemSound(soundIDs[10]);
    shell_delay(125);
}

int shell_low_battery() {
    int lowbat = IOPSGetBatteryWarningLevel() != kIOPSLowBatteryWarningNone;
    if (ann_battery != lowbat) {
        ann_battery = lowbat;
        skin_update_annunciator(5, ann_battery);
     }
    return lowbat;
}

uint4 shell_milliseconds() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint4) (tv.tv_sec * 1000L + tv.tv_usec / 1000);
}

int shell_decimal_point() {
    NSLocale *loc = [NSLocale currentLocale];
    NSString *dec = [loc objectForKey:NSLocaleDecimalSeparator];
    return [dec isEqualToString:@","] ? 0 : 1;
}

void shell_get_time_date(uint4 *time, uint4 *date, int *weekday) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm tms;
    localtime_r(&tv.tv_sec, &tms);
    if (time != NULL)
        *time = ((tms.tm_hour * 100 + tms.tm_min) * 100 + tms.tm_sec) * 100 + tv.tv_usec / 10000;
    if (date != NULL)
        *date = ((tms.tm_year + 1900) * 100 + tms.tm_mon + 1) * 100 + tms.tm_mday;
    if (weekday != NULL)
        *weekday = tms.tm_wday;
}

void shell_delay(int duration) {
    struct timespec ts;
    ts.tv_sec = duration / 1000;
    ts.tv_nsec = (duration % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

uint4 shell_get_mem() {
    // TODO!
    return 42;
}

void shell_blitter(const char *bits, int bytesperline, int x, int y, int width, int height) {
    skin_display_blitter(bits, bytesperline, x, y, width, height);
}

void shell_annunciators(int updn, int shf, int prt, int run, int g, int rad) {
    if (updn != -1 && ann_updown != updn) {
        ann_updown = updn;
        skin_update_annunciator(1, ann_updown);
    }
    if (shf != -1 && ann_shift != shf) {
        ann_shift = shf;
        skin_update_annunciator(2, ann_shift);
    }
    if (prt != -1) {
        [instance print_ann_helper:prt];
    }
    if (run != -1 && ann_run != run) {
        ann_run = run;
        skin_update_annunciator(4, ann_run);
    }
    if (g != -1 && ann_g != g) {
        ann_g = g;
        skin_update_annunciator(6, ann_g);
    }
    if (rad != -1 && ann_rad != rad) {
        ann_rad = rad;
        skin_update_annunciator(7, ann_rad);
    }
}

void shell_powerdown() {
    quit_flag = 1;
    we_want_cpu = 1;
}

void shell_print(const char *text, int length,
                 const char *bits, int bytesperline,
                 int x, int y, int width, int height) {

    int xx, yy;
    int oldlength, newlength;
    
    for (yy = 0; yy < height; yy++) {
        int4 Y = (printout_bottom + 2 * yy) % PRINT_LINES;
        for (xx = 0; xx < 143; xx++) {
            int bit, px, py;
            if (xx < width) {
                char c = bits[(y + yy) * bytesperline + ((x + xx) >> 3)];
                bit = (c & (1 << ((x + xx) & 7))) != 0;
            } else
                bit = 0;
            for (px = xx * 2; px < (xx + 1) * 2; px++)
                for (py = Y; py < Y + 2; py++)
                    if (bit)
                        print_bitmap[py * PRINT_BYTESPERLINE + (px >> 3)]
                        |= 1 << (px & 7);
                    else
                        print_bitmap[py * PRINT_BYTESPERLINE + (px >> 3)]
                        &= ~(1 << (px & 7));
        }
    }
    
    oldlength = printout_bottom - printout_top;
    if (oldlength < 0)
        oldlength += PRINT_LINES;
    printout_bottom = (printout_bottom + 2 * height) % PRINT_LINES;
    newlength = oldlength + 2 * height;
    
    update_params params;
    params.oldlength = oldlength;
    params.newlength = newlength;
    params.height = height;
    [instance.printView updatePrintout:&params];
    
    if (state.printerToTxtFile) {
        int err;
        char buf[1000];
        
        if (print_txt == NULL) {
            print_txt = fopen(state.printerTxtFileName, "a");
            if (print_txt == NULL) {
                err = errno;
                state.printerToTxtFile = 0;
                snprintf(buf, 1000, "Can't open \"%s\" for output:\n%s (%d)\nPrinting to text file disabled.", state.printerTxtFileName, strerror(err), err);
                show_message("Message", buf);
                goto done_print_txt;
            }
        }
        
        if (text != NULL)
            shell_spool_txt(text, length, txt_writer, txt_newliner);
        else
            shell_spool_bitmap_to_txt(bits, bytesperline, x, y, width, height, txt_writer, txt_newliner);
    done_print_txt:;
    }
    
    if (state.printerToGifFile) {
        int err;
        char buf[1000];
        
        if (print_gif != NULL
                && gif_lines + height > state.printerGifMaxLength) {
            shell_finish_gif(gif_seeker, gif_writer);
            fclose(print_gif);
            print_gif = NULL;
        }
        
        if (print_gif == NULL) {
            while (1) {
                int len, p;
                
                gif_seq = (gif_seq + 1) % 10000;
                
                strcpy(print_gif_name, state.printerGifFileName);
                len = strlen(print_gif_name);
                
                /* Strip ".gif" extension, if present */
                if (len >= 4 &&
                    strcasecmp(print_gif_name + len - 4, ".gif") == 0) {
                    len -= 4;
                    print_gif_name[len] = 0;
                }
                
                /* Strip ".[0-9]+", if present */
                p = len;
                while (p > 0 && print_gif_name[p] >= '0'
                       && print_gif_name[p] <= '9')
                    p--;
                if (p < len && p >= 0 && print_gif_name[p] == '.')
                    print_gif_name[p] = 0;
                
                /* Make sure we have enough space for the ".nnnn.gif" */
                p = FILENAMELEN - 10;
                print_gif_name[p] = 0;
                p = strlen(print_gif_name);
                snprintf(print_gif_name + p, 6, ".%04d", gif_seq);
                strcat(print_gif_name, ".gif");
                
                if (!is_file(print_gif_name))
                    break;
            }
            print_gif = fopen(print_gif_name, "w+");
            if (print_gif == NULL) {
                err = errno;
                state.printerToGifFile = 0;
                snprintf(buf, 1000, "Can't open \"%s\" for output:\n%s (%d)\nPrinting to GIF file disabled.", print_gif_name, strerror(err), err);
                show_message("Message", buf);
                goto done_print_gif;
            }
            if (!shell_start_gif(gif_writer, 143, state.printerGifMaxLength)) {
                state.printerToGifFile = 0;
                show_message("Message", "Not enough memory for the GIF encoder.\nPrinting to GIF file disabled.");
                goto done_print_gif;
            }
            gif_lines = 0;
        }
        
        shell_spool_gif(bits, bytesperline, x, y, width, height, gif_writer);
        gif_lines += height;

        if (print_gif != NULL && gif_lines + 9 > state.printerGifMaxLength) {
            shell_finish_gif(gif_seeker, gif_writer);
            fclose(print_gif);
            print_gif = NULL;
        }
        done_print_gif:;
    }

    print_text[print_text_bottom++] = (char) (text == NULL ? 255 : length);
    if (print_text_bottom == PRINT_TEXT_SIZE)
        print_text_bottom = 0;
    if (text != NULL) {
        if (print_text_bottom + length < PRINT_TEXT_SIZE) {
            memcpy(print_text + print_text_bottom, text, length);
            print_text_bottom += length;
        } else {
            int part = PRINT_TEXT_SIZE - print_text_bottom;
            memcpy(print_text + print_text_bottom, text, part);
            memcpy(print_text, text + part, length - part);
            print_text_bottom = length - part;
        }
    }
    print_text_pixel_height += text == NULL ? 16 : 9;
    while (print_text_pixel_height > PRINT_LINES / 2 - 1) {
        int tll = print_text[print_text_top] == 255 ? 16 : 9;
        print_text_pixel_height -= tll;
        print_text_top += tll == 16 ? 1 : (print_text[print_text_top] + 1);
        if (print_text_top >= PRINT_TEXT_SIZE)
            print_text_top -= PRINT_TEXT_SIZE;
    }
}

void shell_request_timeout3(int delay) {
    [instance setTimeout3:delay];
}

void shell_log(const char *message) {
    NSLog(@"%s", message);
}

int shell_wants_cpu() {
    if (we_want_cpu)
        return true;
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec > runner_end_time.tv_sec
        || now.tv_sec == runner_end_time.tv_sec && now.tv_usec >= runner_end_time.tv_usec;
}

static void read_key_map(const char *keymapfilename) {
    FILE *keymapfile = fopen(keymapfilename, "r");
    int kmcap = 0;
    char line[1024];
    int lineno = 0;

    if (keymapfile == NULL) {
        /* Try to create default keymap file */
        keymapfile = fopen(keymapfilename, "wb");
        if (keymapfile == NULL)
            return;
        NSString *path = [[NSBundle mainBundle] pathForResource:@"keymap" ofType:@"txt"];
        [path getCString:line maxLength:1024 encoding:NSUTF8StringEncoding];
        FILE *builtin_keymapfile = fopen(line, "r");
        int n;
        while ((n = fread(line, 1, 1024, builtin_keymapfile)) > 0)
            fwrite(line, 1, n, keymapfile);
        fclose(builtin_keymapfile);
        fclose(keymapfile);

        keymapfile = fopen(keymapfilename, "r");
        if (keymapfile == NULL)
            return;
    }

    while (fgets(line, 1024, keymapfile) != NULL) {
        keymap_entry *entry = parse_keymap_entry(line, ++lineno);
        if (entry == NULL)
            continue;
        /* Create new keymap entry */
        if (keymap_length == kmcap) {
            kmcap += 50;
            keymap = (keymap_entry *) realloc(keymap, kmcap * sizeof(keymap_entry));
            // TODO - handle memory allocation failure
        }
        memcpy(keymap + (keymap_length++), entry, sizeof(keymap_entry));
    }

    fclose(keymapfile);
}

static void init_shell_state(int4 version) {
    switch (version) {
        case -1:
            state.printerToTxtFile = 0;
            state.printerToGifFile = 0;
            state.printerTxtFileName[0] = 0;
            state.printerGifFileName[0] = 0;
            state.printerGifMaxLength = 256;
            state.mainWindowKnown = 0;
            state.printWindowKnown = 0;
            state.skinName[0] = 0;
            /* fall through */
        case 0:
            strcpy(state.coreName, "Untitled");
            /* fall through */
        case 1:
            core_settings.matrix_singularmatrix = false;
            core_settings.matrix_outofrange = false;
            core_settings.auto_repeat = true;
        case 2:
            /* current version (SHELL_VERSION = 2),
             * so nothing to do here since everything
             * was initialized from the state file.
             */
            ;
    }
}

static int read_shell_state(int4 *ver) {
    int4 magic;
    int4 version;
    int4 state_size;
    int4 state_version;
    
    if (fread(&magic, 1, sizeof(int4), statefile) != sizeof(int4))
        return 0;
    if (magic != FREE42_MAGIC)
        return 0;
    
    if (fread(&version, 1, sizeof(int4), statefile) != sizeof(int4))
        return 0;
    if (version == 0) {
        /* State file version 0 does not contain shell state,
         * only core state, so we just hard-init the shell.
         */
        init_shell_state(-1);
        *ver = version;
        return 1;
    }
    
    if (fread(&state_size, 1, sizeof(int4), statefile) != sizeof(int4))
        return 0;
    if (fread(&state_version, 1, sizeof(int4), statefile) != sizeof(int4))
        return 0;
    if (state_version < 0 || state_version > SHELL_VERSION)
    /* Unknown shell state version */
        return 0;
    if (fread(&state, 1, state_size, statefile) != state_size)
        return 0;

    if (state_version >= 2) {
        core_settings.matrix_singularmatrix = state.matrix_singularmatrix;
        core_settings.matrix_outofrange = state.matrix_outofrange;
        core_settings.auto_repeat = state.auto_repeat;
    }
    
    init_shell_state(state_version);
    *ver = version;
    return 1;
}

static int write_shell_state() {
    int4 magic = FREE42_MAGIC;
    int4 version = 27;
    int4 state_size = sizeof(state_type);
    int4 state_version = SHELL_VERSION;
    
    if (fwrite(&magic, 1, sizeof(int4), statefile) != sizeof(int4))
        return 0;
    if (fwrite(&version, 1, sizeof(int4), statefile) != sizeof(int4))
        return 0;
    if (fwrite(&state_size, 1, sizeof(int4), statefile) != sizeof(int4))
        return 0;
    if (fwrite(&state_version, 1, sizeof(int4), statefile) != sizeof(int4))
        return 0;
    state.matrix_singularmatrix = core_settings.matrix_singularmatrix;
    state.matrix_outofrange = core_settings.matrix_outofrange;
    state.auto_repeat = core_settings.auto_repeat;
    if (fwrite(&state, 1, sizeof(state_type), statefile) != sizeof(state_type))
        return 0;
    
    return 1;
}

/* Callbacks used by shell_print() and shell_spool_txt() / shell_spool_gif() */

static void txt_writer(const char *text, int length) {
    int n;
    if (print_txt == NULL)
        return;
    n = fwrite(text, 1, length, print_txt);
    if (n != length) {
        char buf[1000];
        state.printerToTxtFile = 0;
        fclose(print_txt);
        print_txt = NULL;
        snprintf(buf, 1000, "Error while writing to \"%s\".\nPrinting to text file disabled", state.printerTxtFileName);
        show_message("Message", buf);
    }
}   

static void txt_newliner() {
    if (print_txt == NULL)
        return;
    fputc('\r', print_txt);
    fputc('\n', print_txt);
    fflush(print_txt);
}   

static void gif_seeker(int4 pos) {
    if (print_gif == NULL)
        return;
    if (fseek(print_gif, pos, SEEK_SET) == -1) {
        char buf[1000];
        state.printerToGifFile = 0;
        fclose(print_gif);
        print_gif = NULL;
        snprintf(buf, 1000, "Error while seeking \"%s\".\nPrinting to GIF file disabled", print_gif_name);
        show_message("Message", buf);
    }
}

static void gif_writer(const char *text, int length) {
    int n;
    if (print_gif == NULL)
        return;
    n = fwrite(text, 1, length, print_gif);
    if (n != length) {
        char buf[1000];
        state.printerToGifFile = 0;
        fclose(print_gif);
        print_gif = NULL;
        snprintf(buf, 1000, "Error while writing to \"%s\".\nPrinting to GIF file disabled", print_gif_name);
        show_message("Message", buf);
    }
}

static bool is_file(const char *name) {
    struct stat st;
    if (stat(name, &st) == -1)
        return false;
    return S_ISREG(st.st_mode);
}
