/*
 * Copyright (C) 2022 Deniz Eren <deniz.eren@outlook.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>

#include "config.h"
#include "pci.h"


void print_version (void) {
#if COVERAGE_BUILD == 1
    printf("\e[1mdev-can-linux v%s - Coverage Build (NOT PRODUCTION!)\e[m\n",
            PROGRAM_VERSION);
#elif PROFILING_BUILD == 1
    printf("\e[1mdev-can-linux v%s - Profiling Build (NOT PRODUCTION!)\e[m\n",
            PROGRAM_VERSION);
#elif DEBUG_BUILD == 1
    printf("\e[1mdev-can-linux v%s - Debug Build\e[m\n", PROGRAM_VERSION);
#else
    printf("\e[1mdev-can-linux v%s\e[m\n", PROGRAM_VERSION);
#endif

    return;
}

void print_configs (void) {
    print_version();

    printf("CONFIG_HZ=%d\n", CONFIG_HZ);
    printf("CONFIG_QNX_INTERRUPT_ATTACH=%d\n",
            CONFIG_QNX_INTERRUPT_ATTACH);
    printf("CONFIG_QNX_INTERRUPT_ATTACH_EVENT=%d\n",
            CONFIG_QNX_INTERRUPT_ATTACH_EVENT);
    printf("CONFIG_QNX_RESMGR_SINGLE_THREAD=%d\n",
            CONFIG_QNX_RESMGR_SINGLE_THREAD);
    printf("CONFIG_QNX_RESMGR_THREAD_POOL=%d\n",
            CONFIG_QNX_RESMGR_THREAD_POOL);

    return;
}

void print_notice (void) {
    print_version();

    printf("\e[1mdev-can-linux\e[m comes with ABSOLUTELY NO WARRANTY; for details use "
            "option `\e[1m-w\e[m'.\n");
    printf("This is free software, and you are welcome to redistribute it\n");
    printf("under certain conditions; option `\e[1m-c\e[m' for details.\n");
}

void print_driver_selection_results (struct driver_selection_t* ds) {
    if (ds->driver_auto) {
        log_warn("Auto detected device (%x:%x) successfully; (driver \"%s\")\n",
                ds->vid, ds->did, detected_driver->name);
    }
    else if (ds->driver_pick) {
        log_warn("Device (%x:%x) accepted successfully; (driver \"%s\")\n",
                ds->vid, ds->did, detected_driver->name);
    }
    else if (ds->driver_unsupported) {
        log_warn("Device (%x:%x) not supported by any driver\n",
                opt_vid, opt_did);
    }
    else {
        log_warn("Device (%x:%x) not a valid device\n", opt_vid, opt_did);
    }

    if (ds->driver_ignored) {
        log_warn("Note: one or more supported devices have been ignored "
                "because of manual device selection\n");
    }
}

static void print_card (FILE* file, const struct pci_driver* driver) {
    fprintf(file, "  Driver: \e[1m%s\e[m\n", driver->name);
    fprintf(file, "  Supported devices (detailed):\n");

    if (driver->id_table != NULL) {
        const struct pci_device_id *id_table = driver->id_table;

        while (id_table->vendor != 0) {
            fprintf(file, "    { vendor: \e[1m%x\e[m, device: \e[1m%x\e[m, subvendor: %x, "
                                "subdevice: %x, class: %x, class_mask: %x }\n",
                id_table->vendor,
                id_table->device,
                id_table->subvendor,
                id_table->subdevice,
                id_table->class,
                id_table->class_mask);
            ++id_table;
        }
    }
}

void print_support (bool detailed) {
    print_notice();

    printf("\n");

    if (detailed) {
        printf("\e[1mAdvantech PCI cards:\e[m\n");
        print_card(stdout, &adv_pci_driver);

        printf("\e[1mKVASER PCAN PCI cards:\e[m\n");
        print_card(stdout, &kvaser_pci_driver);

        printf("\e[1mEMS CPC-PCI/PCIe/104P CAN cards:\e[m\n");
        print_card(stdout, &ems_pci_driver);

        printf("\e[1mPEAK PCAN PCI family cards:\e[m\n");
        print_card(stdout, &peak_pci_driver);

        printf("\e[1mPLX90xx PCI-bridge cards (with the SJA1000 chips):\e[m\n");
        print_card(stdout, &plx_pci_driver);

        return;
    }

    printf("Supports:\n");
    printf("  - Advantech PCI cards\n");
    printf("  - KVASER PCAN PCI cards\n");
    printf("  - EMS CPC-PCI/PCIe/104P CAN cards\n");
    printf("  - PEAK PCAN PCI family cards\n");
    printf("  - PLX90xx PCI-bridge cards (with the SJA1000 chips)\n");
    printf("\n");
    printf("For more details use option `\e[1m-ii\e[m'\n");
}

void print_help (char* program_name) {
    print_notice();

    printf("\n");
    printf("\e[1mSYNOPSIS\e[m\n");
    printf("    \e[1m%s\e[m [options]\n", program_name);
    printf("\n");
    printf("\e[1mDESCRIPTION\e[m\n");
    printf("    \e[1mDEV-CAN-LINUX\e[m is a QNX CAN-bus driver project that aims at porting drivers\n");
    printf("    from the open-source Linux Kernel project to QNX RTOS.\n");
    printf("\n");
    printf("\e[1mOPTIONS\e[m\n");
    printf("    \e[1m-V\e[m             - print application version and exit\n");
    printf("    \e[1m-C\e[m             - print build configurations and exit\n");
    printf("    \e[1m-i\e[m             - list supported hardware and exit\n");
    printf("    \e[1m-ii\e[m            - list supported hardware details and exit\n");
    printf("    \e[1m-d {vid}:{did}\e[m - target desired device, e.g. -d 13fe:c302\n");
    printf("    \e[1m-w\e[m             - print warranty message and exit\n");
    printf("    \e[1m-c\e[m             - print license details and exit\n");
    printf("    \e[1m-q\e[m             - quiet mode turns of all terminal printing and trumps all\n");
    printf("                     verbose modes. Both stdout and stderr are turned off!\n");
    printf("                     Errors and warnings are printed to stderr normally when\n");
    printf("                     this option is not selected. Logging to syslog is not\n");
    printf("                     impacted by this option.\n");
    printf("    \e[1m-v\e[m             - verbose 1; prints out info to stdout\n");
    printf("    \e[1m-vv\e[m            - verbose 2; prints out info & debug to stdout\n");
#if RELEASE_BUILD != 1
    printf("    \e[1m-vvv\e[m           - verbose 3; prints out info, debug & trace to stdout;\n");
    printf("                     do NOT enable this for general usage, it is only intended for\n");
    printf("                     debugging during development.\n");
#endif
    printf("    \e[1m-l\e[m             - log 1; syslog entries for info\n");
    printf("    \e[1m-ll\e[m            - log 2; syslog entries for info & debug\n");
#if RELEASE_BUILD != 1
    printf("    \e[1m-lll\e[m           - log 3; syslog entries for info, debug & trace; NOT for\n");
    printf("                     general use.\n");
#endif
    printf("    \e[1m-?/h\e[m           - print help menu and exit\n");
    printf("\n");
    printf("\e[1mNOTES\e[m\n");
    printf("      (i) use command slog2info to check output to syslog\n");
    printf("     (ii) stdout is the standard output stream you are reading now on screen\n");
    printf("    (iii) stderr is the standard error stream; by default writes to screen\n");
    printf("     (iv) errors & warnings are logged to syslog & stderr unaffected by verbose\n");
    printf("          modes but silenced by quiet mode\n");
    printf("      (v) \"trace\" level logging is only useful when single messages are sent\n");
    printf("          and received, intended only for testing during implementation of new\n");
    printf("          driver support.\n");
    printf("\n");
    printf("\e[1mEXAMPLES\e[m\n");
    printf("    Run with auto detection of hardware:\n");
    printf("\n");
    printf("        \e[1mdev-can-linux\e[m\n");
    printf("\n");
    printf("    Check syslog for errors & warnings:\n");
    printf("\n");
    printf("        \e[1mslog2info\e[m\n");
    printf("\n");
    printf("    If multiple supported cards are installed, the first supported card will be\n");
    printf("    automatically chosen. To override this behaviour and manually specify the\n");
    printf("    desired device, first find out what the vendor ID (vid) and device ID (did) of\n");
    printf("    the desired card is as follows:\n");
    printf("\n");
    printf("        \e[1mpci-tool -v\e[m\n");
    printf("\n");
    printf("    An example output looks like this:\n");
    printf("\n");
    printf("        B000:D05:F00 @ idx 7\n");
    printf("            vid/did: \e[1m13fe/c302\e[m\n");
    printf("                    <vendor id - unknown>, <device id - unknown>\n");
    printf("            class/subclass/reg: 0c/09/00\n");
    printf("                    CANbus Serial Bus Controller\n");
    printf("\n");
    printf("    In this example we would chose the numbers \e[1mvid=13fe\e[m, \e[1mdid=c302\e[m\n");
    printf("\n");
    printf("    Target specific hardware detection of hardware and enable max verbose mode for\n");
    printf("    debugging:\n");
    printf("\n");
    printf("        \e[1mdev-can-linux -d 13fe:c302 -vvvv\e[m\n");
    printf("\n");
    printf("\e[1mBUGS\e[m\n");
    printf("    If you find a bug, please report it.\n");
}

void print_warranty (void) {
    print_version();

    printf("\n");
    printf("This program is distributed in the hope that it will be useful,\n");
    printf("but WITHOUT ANY WARRANTY; without even the implied warranty of\n");
    printf("MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n");
    printf("GNU General Public License for more details.\n");
    printf("This is free software, and you are welcome to redistribute it\n");
    printf("under certain conditions; option `-c' for details.\n");
    printf("\n");
    printf("NO WARRANTY\n");
    printf("\n");
    printf("    BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY\n");
    printf("FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN\n");
    printf("OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES\n");
    printf("PROVIDE THE PROGRAM \"AS IS\" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED\n");
    printf("OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF\n");
    printf("MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS\n");
    printf("TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE\n");
    printf("PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING,\n");
    printf("REPAIR OR CORRECTION.\n");
    printf("\n");
    printf("    IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING\n");
    printf("WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY AND/OR\n");
    printf("REDISTRIBUTE THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES,\n");
    printf("INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING\n");
    printf("OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED\n");
    printf("TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY\n");
    printf("YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER\n");
    printf("PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE\n");
    printf("POSSIBILITY OF SUCH DAMAGES.\n");
}

void print_license (void) {
    print_version();

    printf("\n");
    printf("    GNU GENERAL PUBLIC LICENSE\n");
    printf("       Version 2, June 1991\n");
    printf("\n");
    printf("Copyright (C) 1989, 1991 Free Software Foundation, Inc.,\n");
    printf("51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA\n");
    printf("Everyone is permitted to copy and distribute verbatim copies\n");
    printf("of this license document, but changing it is not allowed.\n");
    printf("\n");
    printf("            Preamble\n");
    printf("\n");
    printf("The licenses for most software are designed to take away your\n");
    printf("freedom to share and change it.  By contrast, the GNU General Public\n");
    printf("License is intended to guarantee your freedom to share and change free\n");
    printf("software--to make sure the software is free for all its users.  This\n");
    printf("General Public License applies to most of the Free Software\n");
    printf("Foundation's software and to any other program whose authors commit to\n");
    printf("using it.  (Some other Free Software Foundation software is covered by\n");
    printf("the GNU Lesser General Public License instead.)  You can apply it to\n");
    printf("your programs, too.\n");
    printf("\n");
    printf("When we speak of free software, we are referring to freedom, not\n");
    printf("price.  Our General Public Licenses are designed to make sure that you\n");
    printf("have the freedom to distribute copies of free software (and charge for\n");
    printf("this service if you wish), that you receive source code or can get it\n");
    printf("if you want it, that you can change the software or use pieces of it\n");
    printf("in new free programs; and that you know you can do these things.\n");
    printf("\n");
    printf("To protect your rights, we need to make restrictions that forbid\n");
    printf("anyone to deny you these rights or to ask you to surrender the rights.\n");
    printf("These restrictions translate to certain responsibilities for you if you\n");
    printf("distribute copies of the software, or if you modify it.\n");
    printf("\n");
    printf("For example, if you distribute copies of such a program, whether\n");
    printf("gratis or for a fee, you must give the recipients all the rights that\n");
    printf("you have.  You must make sure that they, too, receive or can get the\n");
    printf("source code.  And you must show them these terms so they know their\n");
    printf("rights.\n");
    printf("\n");
    printf("We protect your rights with two steps: (1) copyright the software, and\n");
    printf("(2) offer you this license which gives you legal permission to copy,\n");
    printf("distribute and/or modify the software.\n");
    printf("\n");
    printf("Also, for each author's protection and ours, we want to make certain\n");
    printf("that everyone understands that there is no warranty for this free\n");
    printf("software.  If the software is modified by someone else and passed on, we\n");
    printf("want its recipients to know that what they have is not the original, so\n");
    printf("that any problems introduced by others will not reflect on the original\n");
    printf("authors' reputations.\n");
    printf("\n");
    printf("Finally, any free program is threatened constantly by software\n");
    printf("patents.  We wish to avoid the danger that redistributors of a free\n");
    printf("program will individually obtain patent licenses, in effect making the\n");
    printf("program proprietary.  To prevent this, we have made it clear that any\n");
    printf("patent must be licensed for everyone's free use or not licensed at all.\n");
    printf("\n");
    printf("The precise terms and conditions for copying, distribution and\n");
    printf("modification follow.\n");
    printf("\n");
    printf("    GNU GENERAL PUBLIC LICENSE\n");
    printf("TERMS AND CONDITIONS FOR COPYING, DISTRIBUTION AND MODIFICATION\n");
    printf("\n");
    printf("0. This License applies to any program or other work which contains\n");
    printf("a notice placed by the copyright holder saying it may be distributed\n");
    printf("under the terms of this General Public License.  The \"Program\", below,\n");
    printf("refers to any such program or work, and a \"work based on the Program\"\n");
    printf("means either the Program or any derivative work under copyright law:\n");
    printf("that is to say, a work containing the Program or a portion of it,\n");
    printf("either verbatim or with modifications and/or translated into another\n");
    printf("language.  (Hereinafter, translation is included without limitation in\n");
    printf("the term \"modification\".)  Each licensee is addressed as \"you\".\n");
    printf("\n");
    printf("Activities other than copying, distribution and modification are not\n");
    printf("covered by this License; they are outside its scope.  The act of\n");
    printf("running the Program is not restricted, and the output from the Program\n");
    printf("is covered only if its contents constitute a work based on the\n");
    printf("Program (independent of having been made by running the Program).\n");
    printf("Whether that is true depends on what the Program does.\n");
    printf("\n");
    printf("1. You may copy and distribute verbatim copies of the Program's\n");
    printf("source code as you receive it, in any medium, provided that you\n");
    printf("conspicuously and appropriately publish on each copy an appropriate\n");
    printf("copyright notice and disclaimer of warranty; keep intact all the\n");
    printf("notices that refer to this License and to the absence of any warranty;\n");
    printf("and give any other recipients of the Program a copy of this License\n");
    printf("along with the Program.\n");
    printf("\n");
    printf("You may charge a fee for the physical act of transferring a copy, and\n");
    printf("you may at your option offer warranty protection in exchange for a fee.\n");
    printf("\n");
    printf("2. You may modify your copy or copies of the Program or any portion\n");
    printf("of it, thus forming a work based on the Program, and copy and\n");
    printf("distribute such modifications or work under the terms of Section 1\n");
    printf("above, provided that you also meet all of these conditions:\n");
    printf("\n");
    printf("a) You must cause the modified files to carry prominent notices\n");
    printf("stating that you changed the files and the date of any change.\n");
    printf("\n");
    printf("b) You must cause any work that you distribute or publish, that in\n");
    printf("whole or in part contains or is derived from the Program or any\n");
    printf("part thereof, to be licensed as a whole at no charge to all third\n");
    printf("parties under the terms of this License.\n");
    printf("\n");
    printf("c) If the modified program normally reads commands interactively\n");
    printf("when run, you must cause it, when started running for such\n");
    printf("interactive use in the most ordinary way, to print or display an\n");
    printf("announcement including an appropriate copyright notice and a\n");
    printf("notice that there is no warranty (or else, saying that you provide\n");
    printf("a warranty) and that users may redistribute the program under\n");
    printf("these conditions, and telling the user how to view a copy of this\n");
    printf("License.  (Exception: if the Program itself is interactive but\n");
    printf("does not normally print such an announcement, your work based on\n");
    printf("the Program is not required to print an announcement.)\n");
    printf("\n");
    printf("These requirements apply to the modified work as a whole.  If\n");
    printf("identifiable sections of that work are not derived from the Program,\n");
    printf("and can be reasonably considered independent and separate works in\n");
    printf("themselves, then this License, and its terms, do not apply to those\n");
    printf("sections when you distribute them as separate works.  But when you\n");
    printf("distribute the same sections as part of a whole which is a work based\n");
    printf("on the Program, the distribution of the whole must be on the terms of\n");
    printf("this License, whose permissions for other licensees extend to the\n");
    printf("entire whole, and thus to each and every part regardless of who wrote it.\n");
    printf("\n");
    printf("Thus, it is not the intent of this section to claim rights or contest\n");
    printf("your rights to work written entirely by you; rather, the intent is to\n");
    printf("exercise the right to control the distribution of derivative or\n");
    printf("collective works based on the Program.\n");
    printf("\n");
    printf("In addition, mere aggregation of another work not based on the Program\n");
    printf("with the Program (or with a work based on the Program) on a volume of\n");
    printf("a storage or distribution medium does not bring the other work under\n");
    printf("the scope of this License.\n");
    printf("\n");
    printf("3. You may copy and distribute the Program (or a work based on it,\n");
    printf("under Section 2) in object code or executable form under the terms of\n");
    printf("Sections 1 and 2 above provided that you also do one of the following:\n");
    printf("\n");
    printf("a) Accompany it with the complete corresponding machine-readable\n");
    printf("source code, which must be distributed under the terms of Sections\n");
    printf("1 and 2 above on a medium customarily used for software interchange; or,\n");
    printf("\n");
    printf("b) Accompany it with a written offer, valid for at least three\n");
    printf("years, to give any third party, for a charge no more than your\n");
    printf("cost of physically performing source distribution, a complete\n");
    printf("machine-readable copy of the corresponding source code, to be\n");
    printf("distributed under the terms of Sections 1 and 2 above on a medium\n");
    printf("customarily used for software interchange; or,\n");
    printf("\n");
    printf("c) Accompany it with the information you received as to the offer\n");
    printf("to distribute corresponding source code.  (This alternative is\n");
    printf("allowed only for noncommercial distribution and only if you\n");
    printf("received the program in object code or executable form with such\n");
    printf("an offer, in accord with Subsection b above.)\n");
    printf("\n");
    printf("The source code for a work means the preferred form of the work for\n");
    printf("making modifications to it.  For an executable work, complete source\n");
    printf("code means all the source code for all modules it contains, plus any\n");
    printf("associated interface definition files, plus the scripts used to\n");
    printf("control compilation and installation of the executable.  However, as a\n");
    printf("special exception, the source code distributed need not include\n");
    printf("anything that is normally distributed (in either source or binary\n");
    printf("form) with the major components (compiler, kernel, and so on) of the\n");
    printf("operating system on which the executable runs, unless that component\n");
    printf("itself accompanies the executable.\n");
    printf("\n");
    printf("If distribution of executable or object code is made by offering\n");
    printf("access to copy from a designated place, then offering equivalent\n");
    printf("access to copy the source code from the same place counts as\n");
    printf("distribution of the source code, even though third parties are not\n");
    printf("compelled to copy the source along with the object code.\n");
    printf("\n");
    printf("4. You may not copy, modify, sublicense, or distribute the Program\n");
    printf("except as expressly provided under this License.  Any attempt\n");
    printf("otherwise to copy, modify, sublicense or distribute the Program is\n");
    printf("void, and will automatically terminate your rights under this License.\n");
    printf("However, parties who have received copies, or rights, from you under\n");
    printf("this License will not have their licenses terminated so long as such\n");
    printf("parties remain in full compliance.\n");
    printf("\n");
    printf("5. You are not required to accept this License, since you have not\n");
    printf("signed it.  However, nothing else grants you permission to modify or\n");
    printf("distribute the Program or its derivative works.  These actions are\n");
    printf("prohibited by law if you do not accept this License.  Therefore, by\n");
    printf("modifying or distributing the Program (or any work based on the\n");
    printf("Program), you indicate your acceptance of this License to do so, and\n");
    printf("all its terms and conditions for copying, distributing or modifying\n");
    printf("the Program or works based on it.\n");
    printf("\n");
    printf("6. Each time you redistribute the Program (or any work based on the\n");
    printf("Program), the recipient automatically receives a license from the\n");
    printf("original licensor to copy, distribute or modify the Program subject to\n");
    printf("these terms and conditions.  You may not impose any further\n");
    printf("restrictions on the recipients' exercise of the rights granted herein.\n");
    printf("You are not responsible for enforcing compliance by third parties to\n");
    printf("this License.\n");
    printf("\n");
    printf("7. If, as a consequence of a court judgment or allegation of patent\n");
    printf("infringement or for any other reason (not limited to patent issues),\n");
    printf("conditions are imposed on you (whether by court order, agreement or\n");
    printf("otherwise) that contradict the conditions of this License, they do not\n");
    printf("excuse you from the conditions of this License.  If you cannot\n");
    printf("distribute so as to satisfy simultaneously your obligations under this\n");
    printf("License and any other pertinent obligations, then as a consequence you\n");
    printf("may not distribute the Program at all.  For example, if a patent\n");
    printf("license would not permit royalty-free redistribution of the Program by\n");
    printf("all those who receive copies directly or indirectly through you, then\n");
    printf("the only way you could satisfy both it and this License would be to\n");
    printf("refrain entirely from distribution of the Program.\n");
    printf("\n");
    printf("If any portion of this section is held invalid or unenforceable under\n");
    printf("any particular circumstance, the balance of the section is intended to\n");
    printf("apply and the section as a whole is intended to apply in other\n");
    printf("circumstances.\n");
    printf("\n");
    printf("It is not the purpose of this section to induce you to infringe any\n");
    printf("patents or other property right claims or to contest validity of any\n");
    printf("such claims; this section has the sole purpose of protecting the\n");
    printf("integrity of the free software distribution system, which is\n");
    printf("implemented by public license practices.  Many people have made\n");
    printf("generous contributions to the wide range of software distributed\n");
    printf("through that system in reliance on consistent application of that\n");
    printf("system; it is up to the author/donor to decide if he or she is willing\n");
    printf("to distribute software through any other system and a licensee cannot\n");
    printf("impose that choice.\n");
    printf("\n");
    printf("This section is intended to make thoroughly clear what is believed to\n");
    printf("be a consequence of the rest of this License.\n");
    printf("\n");
    printf("8. If the distribution and/or use of the Program is restricted in\n");
    printf("certain countries either by patents or by copyrighted interfaces, the\n");
    printf("original copyright holder who places the Program under this License\n");
    printf("may add an explicit geographical distribution limitation excluding\n");
    printf("those countries, so that distribution is permitted only in or among\n");
    printf("countries not thus excluded.  In such case, this License incorporates\n");
    printf("the limitation as if written in the body of this License.\n");
    printf("\n");
    printf("9. The Free Software Foundation may publish revised and/or new versions\n");
    printf("of the General Public License from time to time.  Such new versions will\n");
    printf("be similar in spirit to the present version, but may differ in detail to\n");
    printf("address new problems or concerns.\n");
    printf("\n");
    printf("Each version is given a distinguishing version number.  If the Program\n");
    printf("specifies a version number of this License which applies to it and \"any\n");
    printf("later version\", you have the option of following the terms and conditions\n");
    printf("either of that version or of any later version published by the Free\n");
    printf("Software Foundation.  If the Program does not specify a version number of\n");
    printf("this License, you may choose any version ever published by the Free Software\n");
    printf("Foundation.\n");
    printf("\n");
    printf("10. If you wish to incorporate parts of the Program into other free\n");
    printf("programs whose distribution conditions are different, write to the author\n");
    printf("to ask for permission.  For software which is copyrighted by the Free\n");
    printf("Software Foundation, write to the Free Software Foundation; we sometimes\n");
    printf("make exceptions for this.  Our decision will be guided by the two goals\n");
    printf("of preserving the free status of all derivatives of our free software and\n");
    printf("of promoting the sharing and reuse of software generally.\n");
    printf("\n");
    printf("            NO WARRANTY\n");
    printf("\n");
    printf("11. BECAUSE THE PROGRAM IS LICENSED FREE OF CHARGE, THERE IS NO WARRANTY\n");
    printf("FOR THE PROGRAM, TO THE EXTENT PERMITTED BY APPLICABLE LAW.  EXCEPT WHEN\n");
    printf("OTHERWISE STATED IN WRITING THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES\n");
    printf("PROVIDE THE PROGRAM \"AS IS\" WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED\n");
    printf("OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF\n");
    printf("MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE ENTIRE RISK AS\n");
    printf("TO THE QUALITY AND PERFORMANCE OF THE PROGRAM IS WITH YOU.  SHOULD THE\n");
    printf("PROGRAM PROVE DEFECTIVE, YOU ASSUME THE COST OF ALL NECESSARY SERVICING,\n");
    printf("REPAIR OR CORRECTION.\n");
    printf("\n");
    printf("12. IN NO EVENT UNLESS REQUIRED BY APPLICABLE LAW OR AGREED TO IN WRITING\n");
    printf("WILL ANY COPYRIGHT HOLDER, OR ANY OTHER PARTY WHO MAY MODIFY AND/OR\n");
    printf("REDISTRIBUTE THE PROGRAM AS PERMITTED ABOVE, BE LIABLE TO YOU FOR DAMAGES,\n");
    printf("INCLUDING ANY GENERAL, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES ARISING\n");
    printf("OUT OF THE USE OR INABILITY TO USE THE PROGRAM (INCLUDING BUT NOT LIMITED\n");
    printf("TO LOSS OF DATA OR DATA BEING RENDERED INACCURATE OR LOSSES SUSTAINED BY\n");
    printf("YOU OR THIRD PARTIES OR A FAILURE OF THE PROGRAM TO OPERATE WITH ANY OTHER\n");
    printf("PROGRAMS), EVEN IF SUCH HOLDER OR OTHER PARTY HAS BEEN ADVISED OF THE\n");
    printf("POSSIBILITY OF SUCH DAMAGES.\n");
    printf("\n");
    printf("     END OF TERMS AND CONDITIONS\n");
}
