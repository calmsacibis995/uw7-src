#pragma ident "@(#)ca.c	26.1"
#pragma ident "$Header$"

/*
 *
 *      Copyright (C) The Santa Cruz Operation, 1993-1997.
 *      This Module contains Proprietary Information of
 *      The Santa Cruz Operation and should be treated
 *      as Confidential.
 */


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <dirent.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/resmgr.h>
#include <sys/confmgr.h>
#include <sys/nvm.h>
#include <sys/cm_i386at.h>
#include <sys/ca.h>
#include <sys/eisa.h>
#include "common.h"

#if (NETISL == 0)

#define DEVCA "/dev/ca"

char *unknownvendor="unknown vendor";

static char data[EISA_BUFFER_SIZE];   /* PCI: we only want 64 bytes of this
                                       * (although on UW2.1->Gemini BL9 you
                                       * get MAX_PCI_REGISTERS bytes due to bug
                                       */

/*this structure is 64 bytes long and represents PCI config space we can read*/

#pragma pack(1)
struct pciconfig {
 u_short vendor_id;
 u_short device_id;
 u_short cmdreg;
 u_short statusreg;
 u_long  classrev;
 u_char cachelinesize;
 u_char latencytimer;
 u_char header_type;
 u_char bist;
 u_long base0;
 u_long base1;
 u_long base2;
 u_long base3;
 u_long base4;
 u_long base5;
 u_long cardbus;
 u_short subsysvendorid;
 u_short subsystemid;
 u_long rombase;
 u_long reserved1;
 u_long reserved2;
 u_char interruptline;
 u_char interruptpin;
 u_char min_gnt;
 u_char max_lat;
};
#pragma pack()


/* this information from http://www.pcisig.com/siginfo/vendors.html
 * I had to massage it quite a bit from what is posted there
 * It had disclaimer at the bottom: Last modified: Tue May 6 08:37:26 1997
 * Factoid: Some 300 new vendors were added from previous vendors.html
 *          which was last modified Thu Aug 8 13:30:34 1996
 */
struct {
   u_short vendorid;
   char *name;
} vendors[]=
{
{0x1217,"02 MICRO, INC."},
{0x12EC,"3A INTERNATIONAL, INC."},
{0x10B7,"3COM CORPORATION"},
{0x121A,"3DFX INTERACTIVE, INC."},
{0x3D3D,"3DLABS LIMITED"},
{0x1239,"3DO COMPANY"},
{0x117A,"A-TREND TECHNOLOGY"},
{0x1245,"A.P.D., S.A."},
{0x125A,"ABB POWER SYSTEMS"},
{0x10AA,"ACC MICROELECTRONICS"},
{0x1040,"ACCELGRAPHICS INC."},
{0x1113,"ACCTON TECHNOLOGY CORP"},
{0x1025,"ACER INCORPORATED"},
{0x10F2,"ACHME COMPUTER INC."},
{0x11AA,"ACTEL"},
{0x12D9,"ACULAB PLC"},
{0x12B7,"ACUMEN"},
{0x9004,"ADAPTEC"},
{0x120B,"ADAPTIVE SOLUTIONS"},
{0x126B,"ADAX, INC."},
{0x1173,"ADOBE SYSTEMS, INC"},
{0x1238,"ADTRAN"},
{0x1075,"ADVANCED INTEGRATION RES."},
{0x10BC,"ADVANCED LOGIC RESEARCH"},
{0x1022,"ADVANCED MICRO DEVICES"},
{0x10D8,"ADVANCED PERIPHERALS LABS"},
{0x1164,"ADVANCED PERIPHERALS TECH"},
{0x10CD,"ADVANCED SYSTEM PRODUCTS"},
{0x1187,"ADVANCED TECHNOLOGY LABS"},
{0x121B,"ADVANCED TELECOMM MODULES"},
{0x124A,"AEG ELECTROCOM GMBH"},
{0x12CD,"AIMS LAB"},
{0x1096,"ALACRON"},
{0x10F3,"ALARIS, INC."},
{0x1064,"ALCATEL CIT"},
{0x1178,"ALFA, INC."},
{0x12A0,"ALLEN- BRADLEY COMPANY"},
{0x1142,"ALLIANCE SEMICONDUCTOR"},
{0x1259,"ALLIED TELESYN"},
{0x10E9,"ALPS ELECTRIC CO. LTD."},
{0x1237,"ALTA TECHNOLOGY CORP."},
{0x12AE,"ALTEON NETWORKS INC."},
{0x1172,"ALTERA CORPORATION"},
{0x10F8,"ALTOS INDIA LTD"},
{0x1206,"AMDAHL CORPORATION"},
{0x101E,"AMERICAN MEGATRENDS"},
{0x12A7,"AMO GMBH"},
{0x1038,"AMP, INC"},
{0x11D4,"ANALOG DEVICES"},
{0x12D6,"ANALOGIC CORP"},
{0x12BE,"ANCHOR CHIPS"},
{0x1222,"ANCOR COMMUNICATIONS, INC."},
{0x122F,"ANDREW CORPORATION"},
{0x1051,"ANIGMA, INC."},
{0x114C,"ANNABOOKS"},
{0x12DB,"ANNAPOLIS MICRO SYSTEMS,"},
{0x12CB,"ANTEX ELECTRONICS CORP"},
{0x12E5,"APEX INC"},
{0x1097,"APPIAN/ETMA"},
{0x106B,"APPLE COMPUTER INC."},
{0x1213,"APPLIED INTELLIGENT SYST"},
{0x10E8,"APPLIED MICRO CIRCUITS"},
{0x11B1,"APRICOT COMPUTERS"},
{0x10E2,"APTIX CORPORATION"},
{0x121F,"ARCUS TECHNOLOGY, INC."},
{0x1220,"ARIEL CORPORATION"},
{0xEDD8,"ARK LOGIC INC"},
{0x1205,"ARRAY CORPORATION"},
{0x12BC,"ARRAY MICROSYSTEMS"},
{0x10EB,"ARTISTS GRAPHICS"},
{0x1191,"ARTOP ELECTRONIC CORP"},
{0x128A,"ASANTE TECHNOLOGIES, INC."},
{0x10ED,"ASCII CORPORATION"},
{0x125B,"ASIX ELECTRONICS CORPORATION"},
{0x100D,"AST COMPUTER"},
{0x11BF,"ASTRODESIGN, INC."},
{0x1043,"ASUSTEK COMPUTER, INC."},
{0x101A,"AT&T GIS (NCR)"},
{0x11C1,"AT&T MICROELECTRONICS"},
{0x106A,"ATEN RESEARCH INC"},
{0x1002,"ATI TECHNOLOGIES INC"},
{0x1114,"ATMEL CORPORATION"},
{0x1199,"ATTACHMATE CORPORATION"},
{0x117C,"ATTO TECHNOLOGY"},
{0x11D1,"AURAVISION"},
{0x12EB,"AUREAL SEMICONDUCTOR"},
{0x125C,"AURORA TECHNOLOGIES, INC."},
{0x10C2,"AUSPEX SYSTEMS INC."},
{0x10D5,"AUTOLOGIC INC."},
{0x1264,"AVAL NAGASAKI CORPORATION"},
{0x4005,"AVANCE LOGIC INC"},
{0x1289,"AVC TECHNOLOGY, INC."},
{0x11AF,"AVID TECHNOLOGY INC"},
{0x1244,"AVM AUDIOVISUELLES GBH."},
{0x1157,"AVSYS CORPORATION"},
{0x10C4,"AWARD SOFTWARE"},
{0x122D,"AZTECH SYSTEM LTD"},
{0x11A4,"BARCO GRAPHICS NV"},
{0x11B3,"BARR SYSTEMS INC."},
{0x1203,"BAYER CORPORATION, AGFA"},
{0x10D7,"BCM ADVANCED RESEARCH"},
{0x117D,"BECTON DICKINSON"},
{0x10A8,"BENCHMARQ MICROELECTRONICS"},
{0x1118,"BERG ELECTRONICS"},
{0x12D7,"BIOTRONIC SRL"},
{0x108A,"BIT 3 COMPUTER"},
{0x118D,"BITFLOW INC"},
{0x10C0,"BOCA RESEARCH INC."},
{0x1211,"BRAINTECH INC"},
{0x1174,"BRIDGEPORT MACHINES"},
{0x109E,"BROOKTREE CORPORATION"},
{0x12E4,"BROOKTROUT TECHNOLOGY INC"},
{0x119D,"BUG, INC."},
{0x119F,"BULL HN INFORMATION"},
{0x1233,"BUS-TECH, INC."},
{0x104B,"BUSLOGIC"},
{0x123F,"C-CUBE MICROSYSTEMS"},
{0x1193,"CABLETRON"},
{0x10B1,"CABLETRON SYSTEMS INC"},
{0x1086,"CACHE COMPUTER"},
{0x11AC,"CANON INFO. SYS. RESEARCH"},
{0x114B,"CANOPUS CO., LTD"},
{0x10B0,"CARDEXPERT TECHNOLOGY"},
{0x1265,"CASIO COMPUTER CO., LTD."},
{0x1248,"CENTRAL DATA CORPORATION"},
{0x1169,"CENTRE FOR DEV. OF"},
{0x123C,"CENTURY SYSTEMS, INC."},
{0x10DC,"CERN/ECP/EDU"},
{0x10D6,"CETIA"},
{0x1077,"CHAINTECH COMPUTER CO."},
{0x12E0,"CHASE RESEARCH"},
{0x102C,"CHIPS AND TECHNOLOGIES"},
{0x110B,"CHROMATIC RESEARCH INC."},
{0x12E6,"CIREL SYSTEMS"},
{0x1013,"CIRRUS LOGIC"},
{0x1137,"CISCO SYSTEMS INC"},
{0x10CF,"CITICORP TTI"},
{0x106F,"CITY GATE DEVELOPMENT LTD"},
{0x1095,"CMD TECHNOLOGY INC"},
{0x104F,"CO-TIME COMPUTER LTD"},
{0x1109,"COGENT DATA TECHNOLOGIES,"},
{0x12F7,"COGNEX"},
{0xE11,"COMPAQ COMPUTER CORP."},
{0x10DA,"COMPAQ IPG-AUSTIN"},
{0x129D,"COMPCORE MULTIMEDIA, INC."},
{0x120D,"COMPRESSION LABS, INC."},
{0x11F0,"COMPU-SHACK GMBH"},
{0x12BD,"COMPUTERM CORP."},
{0x1130,"COMPUTERVISION"},
{0x11F5,"COMPUTING DEVICES INTL."},
{0x8E0E,"COMPUTONE CORPORATION"},
{0x1277,"COMSTREAM"},
{0x11FE,"COMTROL CORPORATION"},
{0x125F,"CONCURRENT TECHNOLOGIES"},
{0x12C4,"CONNECT TECH INC"},
{0x116B,"CONNECTWARE INC"},
{0x1221,"CONTEC CO., LTD"},
{0x11EC,"CORECO INC"},
{0x10AE,"CORNERSTONE IMAGING"},
{0x118C,"COROLLARY, INC"},
{0x110E,"CPU TECHNOLOGY"},
{0x10F6,"CREATIVE ELECTRONIC SYST"},
{0x1102,"CREATIVE LABS"},
{0x1141,"CREST MICROSYSTEM INC"},
{0x12E8,"CRISC CORP"},
{0x11DD,"CROSFIELD ELECTRONICS LTD"},
{0x127C,"CROSSPOINT SOLUTIONS, INC"},
{0x121E,"CSPI"},
{0x1200,"CSS CORPORATION"},
{0x120E,"CYCLADES CORPORATION"},
{0x113C,"CYCLONE MICROSYSTEMS, INC"},
{0x124E,"CYLINK"},
{0x1080,"CYPRESS SEMICONDUCTOR"},
{0x1078,"CYRIX CORPORATION"},
{0x1186,"D-LINK SYSTEM INC"},
{0x11C7,"D.C.M. DATA SYSTEMS"},
{0x1070,"DAEWOO TELECOM LTD"},
{0x11C6,"DAINIPPON SCREEN MFG. CO."},
{0x10BB,"DAPHA ELECTRONICS CORP."},
{0x1229,"DATA KINESIS INC."},
{0x107F,"DATA TECHNOLOGY CORP."},
{0x1116,"DATA TRANSLATION"},
{0x10B3,"DATABOOK INC"},
{0x1117,"DATACUBE, INC"},
{0x10C9,"DATAEXPERT CORPORATION"},
{0x1185,"DATAWORLD INT'L LTD"},
{0x11FB,"DATEL INC"},
{0x12E2,"DATUM INC. BANCOMM-TIMING"},
{0x1282,"DAVICOM SEMICONDUCTOR, INC"},
{0x12F6,"DAWSON FRANCE"},
{0x1028,"DELL COMPUTER CORPORATION"},
{0x1243,"DELPHAX"},
{0x1192,"DENSAN COMPANY LTD"},
{0x11A3,"DEURETZBACHER GMBH & CO."},
{0x106E,"DFI, INC"},
{0x12D4,"DGM&S"},
{0x12C7,"DIALOGIC CORP"},
{0x1092,"DIAMOND MULTIMEDIA SYSTEMS"},
{0x114F,"DIGI INTERNATIONAL"},
{0x10AB,"DIGICOM"},
{0x1011,"DIGITAL EQUIPMENT CORP."},
{0x11E8,"DIGITAL PROCESSING SYSTEMS"},
{0x1246,"DIPIX TECHNOLOGIES, INC."},
{0x1044,"DISTRIBUTED PROCESSING"},
{0x1068,"DIVERSIFIED TECHNOLOGY"},
{0x11C4,"DOCUMENT TECHNOLOGIES, INC"},
{0x11C8,"DOLPHIN INTERCONNECT SOLUT"},
{0x11EE,"DOME IMAGING SYSTEMS INC"},
{0x1241,"DSC COMMUNICATIONS"},
{0x1139,"DYNAMIC PICTURES, INC"},
{0x11B2,"EASTMAN KODAK"},
{0x111A,"EFFICIENT NETWORKS, INC"},
{0x1133,"EICON TECHNOLOGY CORP."},
{0x116E,"ELECTRONICS FOR IMAGING"},
{0x1048,"ELSA GMBH"},
{0x11EA,"ELSAG BAILEY"},
{0x1120,"EMC CORPORATION"},
{0x10DF,"EMULEX CORPORATION"},
{0x1090,"ENCORE COMPUTER CORPORATION"},
{0x123D,"ENGINEERING DESIGN TEAM,"},
{0x1274,"ENSONIQ"},
{0x12D5,"EQUATOR TECHNOLOGIES"},
{0x113F,"EQUINOX SYSTEMS, INC."},
{0x1262,"ES COMPUTER COMPANY, LTD."},
{0x125D,"ESS TECHNOLOGY"},
{0x120F,"ESSENTIAL COMMUNICATIONS"},
{0x1058,"ETRI"},
{0x10DD,"EVANS & SUTHERLAND"},
{0x1123,"EXCELLENT DESIGN, INC."},
{0x10FE,"FAST MULTIMEDIA AG"},
{0x1129,"FIRMWORKS"},
{0x1094,"FIRST INT'L COMPUTERS"},
{0x1219,"FIRST VIRTUAL CORPORATION"},
{0x1230,"FISHCAMP ENGINEERING"},
{0x1146,"FORCE COMPUTERS"},
{0x1127,"FORE SYSTEMS INC"},
{0x1083,"FOREX COMPUTER CORPORATION"},
{0x12F5,"FORKS"},
{0x1184,"FORKS INC"},
{0x11EB,"FORMATION INC."},
{0x1049,"FOUNTAIN TECHNOLOGIES, INC"},
{0x105B,"FOXCONN INTERNATIONAL, INC"},
{0x1034,"FRAMATOME CONNECTORS USA"},
{0x1135,"FUJI XEROX CO LTD"},
{0x127F,"FUJIFILM"},
{0x12BF,"FUJIFILM MICRODEVICES"},
{0x1183,"FUJIKURA LTD"},
{0x10D0,"FUJITSU LIMITED"},
{0x10CA,"FUJITSU MICROELECTR., INC."},
{0x119E,"FUJITSU MICROELECTRONICS"},
{0x1036,"FUTURE DOMAIN CORP."},
{0x10D1,"FUTUREPLUS SYSTEMS CORP."},
{0x12B4,"FUTURETEL INC"},
{0x113A,"FWB INC"},
{0x12C8,"G FORCE CO, LTD"},
{0x128D,"G2 NETWORKS, INC."},
{0x1197,"GAGE APPLIED SCIENCES, INC"},
{0x10E6,"GAINBERY COMPUTER PRODUCTS"},
{0x11AB,"GALILEO TECHNOLOGY LTD."},
{0x12F2,"GAMMAGRAPHX, INC."},
{0x12B1,"GAMMALINK"},
{0x107B,"GATEWAY 2000"},
{0x12D0,"GDE SYSTEMS, INC."},
{0x12E9,"GE SPACENET"},
{0x11E1,"GEC PLESSEY SEMI INC."},
{0x109B,"GEMLIGHT COMPUTER LTD."},
{0x12B2,"GENERAL SIGNAL NETWORKS"},
{0x1047,"GENOA SYSTEMS CORP"},
{0x12C9,"GIGI OPERATIONS"},
{0x1258,"GILBARCO, INC."},
{0x1072,"GIT CO LTD"},
{0x10A4,"GLOBE MANUFACTURING SALES"},
{0x12C1,"GMM RESEARCH CORP"},
{0x1232,"GPT LIMITED"},
{0x12B5,"GRANITE SYSTEMS INC."},
{0x118F,"GREEN LOGIC"},
{0x124B,"GREENSPRING COMPUTERS INC."},
{0x1253,"GUZIK TECHNICAL ENTERPRISE"},
{0x1271,"GW INSTRUMENTS"},
{0x11CD,"HAL COMPUTER SYSTEMS, INC."},
{0x11A1,"HAMAMATSU PHOTONICS K.K."},
{0x115A,"HARLEQUIN LTD"},
{0x1260,"HARRIS SEMICONDUCTOR"},
{0x112A,"HERMES ELECTRONICS COMPANY"},
{0x118E,"HERMSTEDT GMBH"},
{0x1223,"HEURIKON/COMPUTER PRODUCTS"},
{0x103C,"HEWLETT PACKARD"},
{0x103C,"HEWLETT PACKARD LTD."},
{0x11FD,"HIGH STREET CONSULTANTS"},
{0x118A,"HILEVEL TECHNOLOGY"},
{0x1020,"HITACHI COMPUTER PRODUCTS"},
{0x1037,"HITACHI MICRO SYSTEMS"},
{0x1250,"HITACHI MICROCOMPUTER"},
{0x1054,"HITACHI, LTD"},
{0x1297,"HOLCO ENT CO, LTD/SHUTTLE"},
{0x12C3,"HOLTEK MICROELECTRONICS"},
{0x10AC,"HONEYWELL IAC"},
{0x10AC,"HONEYWELL, INC."},
{0x10D4,"HUALON MICROELECTRONICS"},
{0x1273,"HUGHES NETWORK SYSTEMS"},
{0x1218,"HYBRICON CORP."},
{0x1210,"HYPERPARALLEL TECHNOLOGIES"},
{0x118B,"HYPERTEC PTY LIMITED"},
{0x1196,"HYTEC ELECTRONICS LTD"},
{0x106C,"HYUNDAI ELECTRONICS AMERICA"},
{0x1079,"I-BUS"},
{0x11F9,"I-CUBE INC"},
{0x10FC,"I-O DATA DEVICE, INC."},
{0x1061,"I.I.T."},
{0x1014,"IBM"},
{0x114D,"IC CORPORATION"},
{0x1016,"ICL PERSONAL SYSTEMS"},
{0x10C1,"ICM CO., LTD."},
{0x11E5,"IIX CONSULTING"},
{0x11D5,"IKON CORPORATION"},
{0x129B,"IMAGE ACCESS"},
{0x11D8,"IMAGE TECHNOLOGIES DEVELOP"},
{0x1295,"IMAGENATION CORPORATION"},
{0x112F,"IMAGING TECHNOLOGY, INC"},
{0x1165,"IMAGRAPH CORPORATION"},
{0x12E3,"IMATION CORP-MEDICALIMAGING"},
{0x1035,"INDUSTRIAL TECHNOLOGY RESEA"},
{0x12C0,"INFIMED"},
{0x119C,"INFORMATION TECHNOLOGY INST"},
{0x10A7,"INFORMTECH INDUSTRIAL LTD."},
{0x124F,"INFORTREND TECHNOLOGY, INC."},
{0x105F,"INFOTRONIC AMERICA INC"},
{0x1101,"INITIO CORPORATION"},
{0x11A9,"INNOSYS"},
{0x10EA,"INTEGRAPHICS SYSTEMS"},
{0x117F,"INTEGRATED CIRCUIT SYSTEMS"},
{0x12CA,"INTEGRATED COMPUTING ENGINE"},
{0x111D,"INTEGRATED DEVICE TECH"},
{0x10E0,"INTEGRATED MICRO SOLUTIONS"},
{0x1283,"INTEGRATED TECHNOLOGY EXPR"},
{0x122A,"INTEGRATED TELECOM"},
{0x8086,"INTEL"},
{0x116C,"INTELLIGENT RESOURCES"},
{0x12B3,"INTER-FACE CO LTD"},
{0x1224,"INTERACTIVE IMAGES"},
{0x11D2,"INTERCOM INC."},
{0x1147,"INTERFACE CORP"},
{0x1091,"INTERGRAPH CORPORATION"},
{0x11BE,"INTERNATIONAL"},
{0x107E,"INTERPHASE CORPORATION"},
{0x1140,"INTERVOICE INC"},
{0x1215,"INTERWARE CO., LTD"},
{0x1170,"INVENTEC CORPORATION"},
{0x1046,"IPC CORPORATION, LTD."},
{0x1086,"J. BOND COMPUTER SYSTEMS"},
{0x10D3,"JABIL CIRCUIT INC"},
{0x1151,"JAE ELECTRONICS INC."},
{0x129C,"JAYCOR"},
{0x1100,"JAZZ MULTIMEDIA"},
{0x12B0,"JORGE SCIENTIFIC CORP"},
{0x10A1,"JUKO ELECTRONICS IND. CO"},
{0x11FA,"KASAN ELECTRONICS"},
{0x11F3,"KEITHLEY METRABYTE"},
{0x11F4,"KINETIC SYSTEMS CORP"},
{0x1212,"KINGSTON TECHNOLOGY CORP"},
{0x1299,"KNOWLEDGE TECHNOLOGY LAB"},
{0x1296,"KOFAX IMAGE PRODUCTS"},
{0x12B8,"KORG"},
{0x117B,"L G ELECTRONICS, INC."},
{0x1198,"LAMBDA SYSTEMS INC"},
{0x1153,"LAND WIN ELECTRONIC CORP"},
{0x1204,"LATTICE SEMICONDUCTOR"},
{0x113D,"LEADING EDGE PRODUCTS IN"},
{0x107D,"LEADTEK RESEARCH INC."},
{0x11B4,"LEITCH TECHNOLOGY INTL"},
{0x1124,"LEUTRON VISION AG"},
{0x126A,"LEXMARK INTERNATIONAL, INC."},
{0x107C,"LG ELECTRONICS"},
{0x122B,"LG INDUSTRIAL SYSTEMS CO.,"},
{0x1254,"LINEAR SYSTEMS LTD."},
{0x112B,"LINOTYPE - HELL AG"},
{0x121D,"LIPPERT AUTOMATIONSTECHNIK"},
{0x11AD,"LITE-ON COMMUNICATIONS INC"},
{0x111B,"LITTON GCS"},
{0x3D,"LOCKHEED MARTIN"},
{0x11D0,"LOCKHEED MARTIN FEDERAL SYS"},
{0x1171,"LOUGHBOROUGH SOUND IMAGES PLC"},
{0x1015,"LSI LOGIC CORPORATION"},
{0x11CA,"LSI SYSTEMS, INC"},
{0x12A3,"LUCENT TECHNOLOGIES"},
{0x1287,"M-PACT, INC."},
{0x10D9,"MACRONIX, INC."},
{0x10B6,"MADGE NETWORKS"},
{0x11C9,"MAGMA"},
{0x12DD,"MANAGEMENT GRAPHICS"},
{0x1240,"MARATHON TECHNOLOGIES CORP."},
{0x1062,"MASPAR COMPUTER CORP"},
{0x102B,"MATROX GRAPHICS, INC."},
{0x10F7,"MATSUSHITA ELECTRIC INDUST"},
{0x1189,"MATSUSHITA ELECTRONICS CO LTD"},
{0x1261,"MATSUSHITA-KOTOBUKI ELECTRON"},
{0x115F,"MAXTOR CORPORATION"},
{0x1286,"MAZET GMBH"},
{0x12AC,"MEASUREX CORPORATION"},
{0x1293,"MEDIA REALITY TECHNOLOGY"},
{0x11ED,"MEDIAMATICS"},
{0x109C,"MEGACHIPS CORPORATION"},
{0x1160,"MEGASOFT INC"},
{0x1152,"MEGATEK"},
{0x12F4,"MEGATEL"},
{0x10A0,"MEIDENSHA CORPORATION"},
{0x1154,"MELCO INC"},
{0x12C2,"MENTEC LIMITED"},
{0x10CC,"MENTOR ARC INC"},
{0x1134,"MERCURY COMPUTER SYSTEMS"},
{0x11CC,"MICHELS & KLEBERHOFF COMPUTER"},
{0x10AF,"MICRO COMPUTER SYSYTEMS"},
{0x10E5,"MICRO INDUSTRIES CORP"},
{0x1088,"MICROCOMPUTER SYSTEMS"},
{0x1266,"MICRODYNE CORPORATION"},
{0x1012,"MICRONICS COMPUTERS INC"},
{0x11A5,"MICROUNITY SYSTEMS ENG."},
{0x119A,"MIND SHARE, INC."},
{0x110C,"MINI-MAX TECHNOLOGY,"},
{0x1031,"MIRO COMPUTER PRODUCTS"},
{0x1071,"MITAC"},
{0x12C6,"MITANI CORPORATION"},
{0x1132,"MITEL CORP."},
{0x1175,"MITRON COMPUTER INC."},
{0x1067,"MITSUBISHI"},
{0x10BA,"MITSUBISHI ELECTRIC"},
{0x1067,"MITSUBISHI ELECTRONICS"},
{0x11E6,"MITSUI-ZOSEN SYSTEM"},
{0x10D2,"MOLEX INCORPORATED"},
{0x1136,"MOMENTUM DATA SYSTEMS"},
{0x10BF,"MOST INC"},
{0xC0FE,"MOTION ENGINEERING, INC."},
{0x1057,"MOTOROLA"},
{0x1057,"MOTOROLA COMPUTER GROUP"},
{0x1122,"MULTI-TECH SYSTEMS, INC."},
{0x12AD,"MULTIDATA GMBH"},
{0x1159,"MUTECH CORP"},
{0x1167,"MUTOH INDUSTRIES INC"},
{0x1069,"MYLEX CORPORATION"},
{0x1093,"NATIONAL INSTRUMENTS"},
{0x100B,"NATIONAL SEMICONDUCTOR"},
{0x12B6,"NATURAL MICROSYSTEMS"},
{0x101A,"NCR"},
{0x1291,"NCS COMPUTER ITALIA"},
{0x10FF,"NCUBE"},
{0x1033,"NEC CORPORATION"},
{0x1033,"NEC ELECTRONICS LTD."},
{0x10C8,"NEOMAGIC CORPORATION"},
{0x115D,"NETACCESS"},
{0x11CE,"NETACCESS"},
{0x1007,"NETFRAME SYSTEMS INC"},
{0x1143,"NETPOWER, INC"},
{0x12CE,"NETSPEED INC."},
{0x1275,"NETWORK APPLIANCE CORP"},
{0x113B,"NETWORK COMPUTING DEVICES"},
{0x1202,"NETWORK GENERAL CORP."},
{0x11BC,"NETWORK PERIPHERALS INC"},
{0x107A,"NETWORTH"},
{0x11DF,"NEW WAVE PDG"},
{0x10E3,"NEWBRIDGE MICROSYSTEMS"},
{0x12A2,"NEWGEN SYSTEMS CORPORATI"},
{0x12A8,"NEWS DATACOM"},
{0x1074,"NEXGEN MICROSYSTEMS"},
{0x11EF,"NICOLET TECHNOLOGIES B.V."},
{0x114E,"NIKON SYSTEMS INC"},
{0x12E1,"NINTENDO CO, LTD"},
{0x121C,"NIPPON TEXACO, LTD."},
{0x12BB,"NIPPON UNISOFT CORPORATI"},
{0x10F5,"NKK CORPORATION"},
{0x1228,"NORSK ELEKTRO OPTIKK A/S"},
{0x126C,"NORTHERN TELECOM"},
{0x11DA,"NOVELL"},
{0x12A4,"NTT ELECTRONICS TECHNOL"},
{0x105D,"NUMBER 9 VISUAL TECHNOLOG"},
{0x10DE,"NVIDIA CORPORATION"},
{0x12D2,"NVIDIA/SGS THOMSON"},
{0x1162,"OA LABORATORY CO LTD"},
{0x104E,"OAK TECHNOLOGY, INC"},
{0x108C,"OAKLEIGH SYSTEMS INC."},
{0x1063,"OCEAN OFFICE AUTOMATION"},
{0x129F,"OEC MEDICAL SYSTEMS, INC."},
{0x1021,"OKI ELECTRIC INDUSTRY CO."},
{0x108D,"OLICOM"},
{0x102E,"OLIVETTI ADVANCED TECHNOL"},
{0x1270,"OLYMPUS OPTICAL CO., LTD."},
{0x119B,"OMEGA MICRO INC."},
{0x10CB,"OMRON CORPORATION"},
{0x1045,"OPTI INC."},
{0x1255,"OPTIBASE LTD"},
{0x12ED,"OPTIVISION INC."},
{0x12EE,"ORANGE MICRO"},
{0x109A,"PACKARD BELL"},
{0x1084,"PARADOR"},
{0x115B,"PARALLAX GRAPHICS"},
{0x1208,"PARSYTEC GMBH"},
{0x11B9,"PATHLIGHT TECHNOLOGY INC"},
{0x10F9,"PC DIRECT"},
{0x1042,"PC TECHNOLOGY INC"},
{0x12F0,"PENTEK"},
{0x1256,"PERCEPTIVE SOLUTIONS, INC."},
{0x1214,"PERFORMANCE TECHNOLOGIES,"},
{0x12D8,"PERICOM SEMICONDUCTOR"},
{0x1156,"PERISCOPE ENGINEERING"},
{0x10F0,"PERITEK CORPORATION"},
{0x1161,"PFU LIMITED"},
{0x1131,"PHILIPS SEMICONDUCTORS"},
{0x100A,"PHOENIX TECHNOLOGIES"},
{0x1280,"PHOTOSCRIPT GROUP LTD."},
{0x115C,"PHOTRON LTD."},
{0x1066,"PICOPOWER TECHNOLOGY"},
{0x12C5,"PICTURE ELEMENTS"},
{0x11F2,"PICTURE TEL JAPAN K.K."},
{0x101F,"PICTURETEL"},
{0x1155,"PINE TECHNOLOGY LTD"},
{0x11BD,"PINNACLE SYSTEMS INC."},
{0x11CF,"PIONEER ELECTRONIC"},
{0x127B,"PIXERA CORPORATION"},
{0x1285,"PLATFORM TECHNOLOGIES, INC."},
{0x12CC,"PLUTO TECHNOLOGIES INTERNAT"},
{0x10B5,"PLX TECHNOLOGY, INC."},
{0x12BA,"PMC SIERRA"},
{0x11F8,"PMC-SIERRA INC"},
{0x116A,"POLARIS COMMUNICATIONS"},
{0x11A7,"POWER COMPUTING CORP."},
{0x1225,"POWER I/O, INC."},
{0x11F6,"POWERMATIC DATA SYSTEMS"},
{0x111F,"PRECISION DIGITAL IMAGES"},
{0x11AF,"PRO-LOG CORPORATION"},
{0x105A,"PROMISE TECHNOLOGY, INC."},
{0x12CF,"PROPHET SYSTEMS, INC."},
{0x1108,"PROTEON, INC."},
{0x12D1,"PSITECH"},
{0x11A6,"PURE DATA LTD."},
{0x1216,"PURUP PREPRESS A/S"},
{0x11BB,"PYRAMID TECHNOLOGY"},
{0x1077,"Q LOGIC"},
{0x10A2,"QUANTUM CORPORATION"},
{0x1098,"QUANTUM DESIGNS (H.K.)"},
{0x11DC,"QUESTRA CORPORATION"},
{0x11E3,"QUICKLOGIC CORPORATION"},
{0x10A6,"RACAL INTERLAN"},
{0x10EF,"RACORE COMPUTER PRODUCTS"},
{0x1081,"RADIUS, INC."},
{0x12F3,"RADSTONE TECHNOLOGY"},
{0x11B5,"RADSTONE TECHNOLOGY PLC"},
{0x12DE,"RAINBOW TECHNOLOGIES"},
{0x10C6,"RAMBUS INC."},
{0x1104,"RASTEROPS CORP."},
{0x1195,"RATOC SYSTEM INC"},
{0x112D,"RAVICAD"},
{0x10B2,"RAYTHEON COMPANY"},
{0x10EC,"REALTEK SEMICONDUCTOR"},
{0x1163,"RENDITION"},
{0x1006,"REPLY GROUP"},
{0x128C,"RETIX CORPORATION"},
{0x1294,"RHETOREX, INC."},
{0x1180,"RICOH CO LTD"},
{0x1235,"RISQ MODULAR SYSTEMS"},
{0x1112,"RNS"},
{0x127A,"ROCKWELL INTERNATIONAL"},
{0x10DB,"ROHM LSI SYSTEMS, INC."},
{0x1166,"ROSS COMPUTER"},
{0x110F,"ROSS TECHNOLOGY"},
{0x10F4,"S-MOS SYSTEMS"},
{0x1267,"S. A. TELECOMMUNICATIONS"},
{0x5333,"S3 INC."},
{0x1284,"SAHARA NETWORKS, INC."},
{0x128E,"SAMHO MULTI TECH LTD."},
{0x1099,"SAMSUNG ELECTRONICS CO., LTD"},
{0x1249,"SAMSUNG ELECTRONICS CO., LTD"},
{0x11E2,"SAMSUNG INFORMATION SYSTEMS"},
{0x10C3,"SAMSUNG SEMICONDUCTOR , INC."},
{0x11C2,"SAND MICROELECTRONICS"},
{0x1111,"SANTA CRUZ OPERATION"},
{0x113E,"SANYO ELECTRIC CO"},
{0x1176,"SBE INCORPORATED"},
{0x12DF,"SBS TECHNOLOGIES INC"},
{0x12A6,"SCALABLE NETWORKS, INC."},
{0x1209,"SCI SYSTEMS INC"},
{0x11F7,"SCIENTIFIC ATLANTA"},
{0x11FF,"SCION CORPORATION"},
{0x11AE,"SCITEX CORPORATION"},
{0x12AA,"SDL COMMUNIATIONS, INC."},
{0x11E4,"SECOND WAVE INC"},
{0x123B,"SEEQ TECHNOLOGY, INC."},
{0x11DB,"SEGA ENTERPRISES LTD"},
{0x103A,"SEIKO EPSON CORPORATION"},
{0x106D,"SEQUENT COMPUTER SYSTEMS"},
{0x104A,"SGS THOMSON MICROELECTRONICS"},
{0x1188,"SHIMA SEIKI MANUFACTURING"},
{0x11C5,"SHIVA CORPORATION"},
{0x122C,"SICAN GMBH"},
{0x110A,"SIEMENS NIXDORF AG"},
{0x11A2,"SIERRA RESEARCH AND TECH."},
{0x10A8,"SIERRA SEMICONDUCTOR"},
{0x1236,"SIGMA DESIGNS CORPORATION"},
{0x1105,"SIGMA DESIGNS, INC"},
{0x1177,"SILICON ENGINEERING"},
{0x10A9,"SILICON GRAPHICS"},
{0x1039,"SILICON INTEGRATED SYSTEMS"},
{0x8888,"SILICON MAGIC"},
{0x126F,"SILICON MOTION, INC."},
{0x12A1,"SIMPACT ASSOCIATES, INC."},
{0x123E,"SIMUTECH, INC."},
{0x124C,"SOLITRON TECHNOLOGIES, INC."},
{0x103E,"SOLLIDAY ENGINEERING"},
{0x1263,"SONIC SOLUTIONS"},
{0x104D,"SONY CORPORATION"},
{0x1290,"SORD COMPUTER CORPORATION"},
{0x12F1,"SORENSON VISION INC"},
{0x10FD,"SOYO COMPUTER, INC"},
{0x1017,"SPEA SOFTWARE AG"},
{0x11CB,"SPECIALIX RESEARCH LTD"},
{0x125E,"SPECIALVIDEO ENGINEERING SRL"},
{0x1298,"SPELLCASTER TELECOMMUNICATI"},
{0x126D,"SPLASH TECHNOLOGY, INC."},
{0x124D,"STALLION TECHNOLOGIES, INC."},
{0x10B8,"STANDARD MICROSYSTEMS"},
{0x10B4,"STB SYSTEMS INC"},
{0x1107,"STRATUS COMPUTER"},
{0x126E,"SUMITOMO METAL INDUSTRIES"},
{0x108E,"SUN MICROSYSTEMS COMPUTER"},
{0x12E7,"SUNSGROUP CORPORATION"},
{0x10BD,"SURECOM TECHNOLOGY"},
{0x1276,"SWITCHED NETWORK TECHNOLOG"},
{0x11F1,"SYMBIOS LOGIC INC"},
{0x12DC,"SYMICRON COMPUTER COMMUNICAT"},
{0x10AD,"SYMPHONY LABS"},
{0x120A,"SYNAPTEL"},
{0x103F,"SYNOPSYS/LOGIC MODELING"},
{0x1148,"SYSKONNECT"},
{0x11A8,"SYSTECH CORP."},
{0x108F,"SYSTEMSOFT"},
{0x117E,"T/R SYSTEMS"},
{0x10E4,"TANDEM COMPUTERS"},
{0x128F,"TATENO DENNOU, INC."},
{0x103B,"TATUNG CO. OF AMERICA"},
{0x12AF,"TDK USA CORP"},
{0x11D9,"TEC CORPORATION"},
{0x1227,"TECH-SOURCE"},
{0x1234,"TECHNICAL CORP."},
{0x11D6,"TEKELEC TELECOM"},
{0x1059,"TEKNOR MICROSYSTEMS"},
{0x10E1,"TEKRAM TECHNOLOGY"},
{0x1268,"TEKTRONIX"},
{0x1272,"TELEMATICS"},
{0x1272,"TELEMATICS INTERNATIONAL"},
{0x104C,"TEXAS INSTRUMENTS"},
{0x1065,"TEXAS MICROSYSTEMS"},
{0x10FB,"THESYS GES. F. MIKROELEKTRON"},
{0x1168,"THINE ELECTRONICS INC"},
{0x1150,"THINKING MACHINES CORP"},
{0x1269,"THOMSON-CSF/TTM"},
{0x1288,"TIMESTEP CORPORATION"},
{0x1030,"TMC RESEARCH"},
{0x102F,"TOSHIBA AMERICA INFO"},
{0x1179,"TOSHIBA AMERICA INFO"},
{0x102F,"TOSHIBA AMERICA, ELEC COMP."},
{0x1194,"TOUCAN TECHNOLOGY"},
{0x11D3,"TRANCELL SYSTEMS INC"},
{0x1279,"TRANSMETA CORPORATION"},
{0x1278,"TRANSTECH PARALLEL SYSTEMS"},
{0x128B,"TRANSWITCH CORPORATION"},
{0x11D7,"TRENTON TECHNOLOGY, INC."},
{0x111C,"TRICORD SYSTEMS"},
{0x1023,"TRIDENT MICROSYSTEMS"},
{0x109F,"TRIGEM COMPUTER INC."},
{0x1103,"TRIONES TECHNOLOGIES, INC"},
{0x1292,"TRITECH MICROELECTRONIC"},
{0x12DA,"TRUE TIME"},
{0x10FA,"TRUEVISION"},
{0x100C,"TSENG LABS INC"},
{0x10BE,"TSENGLABS INTERNATIONAL"},
{0x1085,"TULIP COMPUTERS INT.B.V"},
{0x10F1,"TYAN COMPUTER"},
{0x1003,"ULSI SYSTEMS"},
{0x4680,"UMAX COMPUTER CORP"},
{0x1018,"UNISYS CORPORATION"},
{0x1060,"UNITED MICROELECTRONICS"},
{0x11B6,"UNITED VIDEO CORP"},
{0x12B9,"US ROBOTICS"},
{0x11B0,"V3 SEMICONDUCTOR INC."},
{0x10E7,"VADEM"},
{0x127D,"VELA RESEARCH"},
{0x1257,"VERTEX NETWORKS, INC."},
{0x1106,"VIA TECHNOLOGIES, INC."},
{0x1144,"VICKERS, INC."},
{0x129E,"VICTOR COMPANY OF JAPAN"},
{0x1010,"VIDEO LOGIC LTD"},
{0x1335,"VIDEOMAIL, INC."},
{0x11BA,"VIDEOTRON CORP"},
{0x12EF,"VIENNA SYSTEMS"},
{0x12D3,"VINGMED SOUND A/S"},
{0x123A,"VISICOM LABORATORIES"},
{0x12A5,"VISION DYNAMICS LTD."},
{0x1201,"VISTA CONTROLS CORP"},
{0x101B,"VITESSE SEMICONDUCTOR"},
{0x1251,"VLSI SOLUTION OY"},
{0x1004,"VLSI TECHNOLOGY INC"},
{0x129A,"VMETRO, INC."},
{0x114A,"VMIC"},
{0x1158,"VORAX R & D INC"},
{0x1119,"VORTEX COMPUTERSYSTEME"},
{0x105E,"VTECH COMPUTERS LTD"},
{0x100E,"WEITEK"},
{0x111E,"WESTERN DIGITAL"},
};

#define MAXVENDORS 64 * 1024 /* largest integer formed with sizeof u_short */
char *pcivendor[MAXVENDORS];  /* 1:1 hash to speed up searches */

void
InitPCIVendor(void)
{
   register int loop;
   register int numvendors=sizeof(vendors) / sizeof(vendors[0]);

   /* first, initialize all of pcivendor to unknown */
   for (loop=0;loop < MAXVENDORS;loop++) {
      pcivendor[loop]=unknownvendor;
   }

   /* now fill in those we know from table above */
   for (loop=0;loop<numvendors;loop++) {
      pcivendor[vendors[loop].vendorid]=vendors[loop].name;
   }
   return;
}

int
ca_print_i2o(rm_key_t rmkey, u_int longoutput, u_int devconfig)
{
   /* don't need to read the resmgr here */
   /* CM_CA_DEVCONFIG is set
    * to a number that uniquely identifies the I2O object in the system.
    * In particular, it is comprised of the IOP number, a version control
    * field and the object's unique number within the IOP(the TID).
    *
    * CM_CA_DEVCONFIG will be used to uniquely identify
    * an object in I2O space. It will constructed as
    * followes, number of bits for each field in ()
    *
    *    High bit --- > low bit
    *  Global IOP num (16), version control (4), TID (12)
    *   devconfig = Iop << 16 | DevCfgVersion << 12 | lct->LocalTID;
    */
   if (longoutput == 0) {
      char key[10];
      char iop[10];
      char version[10];
      char tid[10];
      snprintf(key,10,"%d",rmkey);
      snprintf(iop,10,"0x%x",(devconfig >> 16) & 0xffff);
      snprintf(version,10,"0x%x",(devconfig & 0xf000) >> 12);
      snprintf(tid,10,"0x%x",devconfig & 0xfff);
      StartList(8,"KEY",10,"IOP",10,"VERSION",10,"TID",10);
      AddToList(4,key,iop,version,tid);
      EndList();
   } else {
      Pstdout("Resmgr key : %d\n",rmkey);
      Pstdout("IOP Number : 0x%x\n",(devconfig >> 16) & 0xffff);
      Pstdout("Version    : 0x%x\n",(devconfig & 0xf000) >> 12);
      Pstdout("Unique IOP#: 0x%x\n",devconfig & 0xfff);
   }
   return(0);
}

int 
ca_print_pci(rm_key_t rmkey, u_int longoutput, u_int devconfig)
{
   int cafd;
   struct cadata nvm;
   struct pciconfig *p=(struct pciconfig *)data;
   /* bus is 8 bits, device is 5 bits, and function is 3 bits */
   u_int bus,device,function,cgnum;
   u_int classcode,subclasscode,interface;
   union {
      /* from struct bus_access in ca.h */
      struct PCIba0 {
         uchar_t  busnumber;
         uchar_t  devfuncnumber;   /* see above comment */
         ushort_t cgnum;           /* was reserved in UW2.1 */
      } PCIba0;
      ulong_t  pciba;
   } PCIba;


   /* if open fails with EBUSY we should wait like we do when opening resmgr */
   if ((cafd=open(DEVCA,O_RDONLY)) == -1) {
      error(NOTBCFG,"ca_print_pci: couldn't open %s, errno=%d",DEVCA,errno);
      return(-1);
   }

   PCIba.pciba=devconfig;
   bus      = PCIba.PCIba0.busnumber;
   function = PCIba.PCIba0.devfuncnumber & 7;   /* low 3 bits */
   device   = PCIba.PCIba0.devfuncnumber & 248; /* high 5 bits */
   cgnum    = PCIba.PCIba0.cgnum;

   nvm.ca_busaccess = devconfig;
   nvm.ca_buffer = data;
   nvm.ca_size = 0;  /* only pertinant to eisa */

   /* NOTE:  UW2.1->Gemini BL9: uts/io/autoconf/ca/ca.c reads 
    * MAX_PCI_REGISTERS(256) bytes from PCI config space and does a copyout of 
    * MAX_PCI_REGISTERS(256) bytes to data[].  This is wrong; you can only
    * refer to the first 64 bytes in PCI space (which is sizeof struct 
    * pciconfig above).  Later Gemini BL10+ only reads 64 bytes of config
    * space and does a copyout of 64 bytes as well.
    * So don't be surprised if machine panics on next line if running
    * this on an earlier release...
    */

   if (ioctl(cafd, CA_PCI_READ_CFG, &nvm) == -1) {
      error(NOTBCFG,"ioctl to read pci config space failed");
      close(cafd);
      return(-1);
   }
 
   close(cafd);
   if (longoutput == 0) {
      /* emulate the old OSR5 pcislot command output with addition of rmkey 
       * Note that we could have obtained all of this except revision
       * from the resmgr and not involving /dev/ca at all
       */
      char rmkeyasstr[10];
      char busasstr[10];
      char deviceasstr[10];
      char functionasstr[10];
      char vendoridasstr[7];
      char deviceidasstr[7];
      char revisionasstr[4];
      char cgnumasstr[6];

      StartList(18,"KEY",4,"BUS",4,"DEVICE",7,"FUNCTION",9,"CGNUM",6,
                   "VENDID",7,"DEVID",7,"REV",4,"VENDOR",30);
      snprintf(rmkeyasstr,10,"%d",rmkey);
      snprintf(busasstr,10,"%x",bus);
      snprintf(deviceasstr,10,"%x",device);
      snprintf(functionasstr,10,"%x",function);
      snprintf(cgnumasstr,6,"%d",cgnum);
      snprintf(vendoridasstr,7,"%04x",p->vendor_id);
      snprintf(deviceidasstr,7,"%04x",p->device_id);
      snprintf(revisionasstr,4,"%d",p->classrev & 0xff);
     
      /* Pstdout("%x %x %x %x %x%x%x\n",rmkey,bus,device,function,
       *     p->vendor_id,p->device_id,p->classrev & 0xff);
       */
      AddToList(9,rmkeyasstr,busasstr,deviceasstr,functionasstr,cgnumasstr,
                  vendoridasstr,deviceidasstr,revisionasstr,
                  pcivendor[p->vendor_id]);
    
      EndList();
   } else {
      Pstdout("resmgr key=%d\n",rmkey);
      Pstdout("vendor id=0x%04x=%s\n",p->vendor_id,pcivendor[p->vendor_id]);
      Pstdout("device id=0x%04x\n",p->device_id);
      Pstdout("command register=0x%x=%s%s%s%s%s%s%s%s%s%s\n",p->cmdreg,
             p->cmdreg & 0x1   ? "I/O Access Enable, ":"",
             p->cmdreg & 0x2   ? "Memory Access Enable, ":"",
             p->cmdreg & 0x4   ? "Master Enable, ":"",
             p->cmdreg & 0x8   ? "Special Cycle Recognition, ":"",
             p->cmdreg & 0x10  ? "Memory Write & Invalidate Enable, ":"",
             p->cmdreg & 0x20  ? "VGA Palette Snoop Enable, ":"",
             p->cmdreg & 0x40  ? "Parity Error Response, ":"",
             p->cmdreg & 0x80  ? "Wait Cycle Enable, ":"",
             p->cmdreg & 0x100 ? "System Error Enable, ":"",
             p->cmdreg & 0x200 ? "Fast Back-to-Back Enable, ":"");
      Pstdout("status register=0x%x=%s%s%s%s%s%s%s%s%s",p->statusreg,
             p->statusreg & 0x20     ? "66MHz-capable, ":"",
             p->statusreg & 0x40     ? "UDF Supported, ":"",
             p->statusreg & 0x80     ? "Fast Back-to-Back Capable, ":"",
             p->statusreg & 0x100    ? "Data Parity Reported, ":"",
              /* 0x200 and 0x400 are Device Select, handled below */
             p->statusreg & 0x800    ? "Signaled Target Abort, ":"",
             p->statusreg & 0x1000   ? "Received Target Abort, ":"",
             p->statusreg & 0x2000   ? "Received Master Abort, ":"",
             p->statusreg & 0x4000   ? "Signaled System Error, ":"",
             p->statusreg & 0x8000   ? "Detected Parity Error, ":"");
      if ((p->statusreg & 0x600) == 0x0) 
         Pstdout("DEVSEL# timing=fast\n");
      else if ((p->statusreg & 0x600) == 0x200)
         Pstdout("DEVSEL# timing=medium\n");
      else if ((p->statusreg & 0x600) == 0x400)
         Pstdout("DEVSEL# timing=slow\n");
      else if ((p->statusreg & 0x600) == 0x600)
         Pstdout("DEVSEL# timing=reserved(0x600)\n");

      Pstdout("Revision ID=0x%x\n",p->classrev & 0xff);
      /* Pstdout("Revision ID=0x%x\n",*((char *)p+8)); */
      Pstdout("Class Code=0x%x=",(p->classrev & 0xffffff00) >> 8);

      classcode=   (p->classrev & 0xff000000) >> 24;
      subclasscode=(p->classrev & 0x00ff0000) >> 16;
      interface=   (p->classrev & 0x0000ff00) >> 8;

      /*
       * Pstdout("classcode=0x%x subclasscode=0x%x interface=0x%x\n",
       *        classcode,subclasscode,interface);
       */

      if (classcode == 0x0) {
         Pstdout("Pre-PCI2.0 device - ");
         if ((subclasscode == 0x0) && (interface == 0x0)) 
            Pstdout("All devices other than VGA");
         else if ((subclasscode == 0x1) && (interface == 0x1))
            Pstdout("VGA-compatible device");
      } else
      if (classcode == 0x1) {
         Pstdout("Mass storage controller - ");
         if ((subclasscode == 0x0) && (interface == 0x0))
            Pstdout("SCSI controller");
         else if (subclasscode == 0x1) {
            Pstdout("IDE controller - ");
            if (interface & 0x1) Pstdout("Operating mode(primary)");
            if (interface & 0x2) Pstdout("Programmable indicator(primary)"); 
            if (interface & 0x4) Pstdout("Operating mode(secondary)");
            if (interface & 0x8) Pstdout("Programmable indicator(secondary)");
            /* 0x10, 0x20, 0x40 reserved */
            if (interface & 0x80) Pstdout("Master IDE device");
         }
      } else
      if (classcode == 0x2) {
         Pstdout("Network controller - ");
         if ((subclasscode == 0x00) && (interface == 0x00))
            Pstdout("Ethernet controller");
         else if ((subclasscode == 0x01) && (interface == 0x00))
            Pstdout("Token ring contoller");
         else if ((subclasscode == 0x02) && (interface == 0x00))
            Pstdout("FDDI controller");
         else if ((subclasscode == 0x03) && (interface == 0x00))
            Pstdout("ATM controller");  
         else if ((subclasscode == 0x80) && (interface == 0x00))
            Pstdout("Other network controller");
      } else
      if (classcode == 0x3) {
         Pstdout("Display controller - ");
         if ((subclasscode == 0x0) && (interface == 0x0)) 
            Pstdout("VGA compatible controller");
         else if ((subclasscode == 0x0) && (interface == 0x1))
            Pstdout("8514-compatible controller");
         else if ((subclasscode == 0x1) && (interface == 0x0))
            Pstdout("XGA controller");
         else if ((subclasscode == 0x80) && (interface == 0x0))
            Pstdout("Other display controller");
      } else
      if (classcode == 0x4) {
         Pstdout("Multimedia device - ");
         if ((subclasscode == 0x0) && (interface == 0x0))
            Pstdout("Video device");
         else if ((subclasscode == 0x1) && (interface == 0x0))
            Pstdout("Audio device");
         else if ((subclasscode == 0x80) && (interface == 0x0))
            Pstdout("Other multimedia device");
      } else
      if (classcode == 0x5) {
         Pstdout("Memory controller - ");
         if ((subclasscode == 0x0) && (interface == 0x0))
            Pstdout("RAM memory controller");
         else if ((subclasscode == 0x1) && (interface == 0x0))
            Pstdout("Flash memory controller");
         else if ((subclasscode == 0x80) && (interface == 0x0))
            Pstdout("Other memory controller");
      } else
      if (classcode == 0x6) {
         Pstdout("Bridge device - ");
         if ((subclasscode == 0x0) && (interface == 0x0))
            Pstdout("Host/PCI bridge");
         else if ((subclasscode == 0x1) && (interface == 0x0))
            Pstdout("PCI/ISA bridge");
         else if ((subclasscode == 0x2) && (interface == 0x0))
            Pstdout("PCI/EISA bridge");
         else if ((subclasscode == 0x3) && (interface == 0x0))
            Pstdout("PCI/MicroChannel bridge");
         else if ((subclasscode == 0x4) && (interface == 0x0))
            Pstdout("PCI/PCI bridge");
         else if ((subclasscode == 0x5) && (interface == 0x0))
            Pstdout("PCI/PCMCIA bridge");
         else if ((subclasscode == 0x6) && (interface == 0x0))
            Pstdout("PCI/NuBus bridge"); 
         else if ((subclasscode == 0x7) && (interface == 0x0))
            Pstdout("PCI/CardBus bridge");
         else if ((subclasscode == 0x80) && (interface == 0x0))
            Pstdout("Other bridge type");
      } else 
      if (classcode == 0x7) {
         Pstdout("Simple communications controller - ");
         if (subclasscode == 0) {
            if (interface == 0x0) 
               Pstdout("Generic XT-compatible serial controller");
            else if (interface == 0x1)
               Pstdout("16450-compatible serial controller");
            else if (interface == 0x2)
               Pstdout("16550-compatible serial controller");
         } else
         if (subclasscode == 0x1) {
            if (interface == 0x0) 
               Pstdout("Parallel port");
            else if (interface == 0x1)
               Pstdout("Bi-directional parallel port");
            else if (interface == 0x2)
               Pstdout("ECP 1.x-compliant parallel port");
         } else
         if ((subclasscode == 0x80) && (interface == 0x0))
            Pstdout("Other communications device");
      } else 
      if (classcode == 0x8) {
         Pstdout("Base system peripheral - ");
         if (subclasscode == 0x0) {
            if (interface == 0x0) 
               Pstdout("Generic 8259 programmable interrupt controller(PIC)");
            else if (interface == 0x1)
               Pstdout("ISA PIC");
            else if (interface == 0x2) 
               Pstdout("EISA PIC");
         } else 
         if (subclasscode == 0x1) {
            if (interface == 0x0)
               Pstdout("Generic 8237 DMA controller");
            else if (interface == 0x1)
               Pstdout("ISA DMA controller");
            else if (interface == 0x2)
               Pstdout("EISA DMA controller");
         } else
         if (subclasscode == 0x2) {
            if (interface == 0x0)
               Pstdout("Generic 8254 timer");
            else if (interface == 0x1)
               Pstdout("ISA system timers");
            else if (interface == 0x2)
               Pstdout("EISA system timers");
         } else
         if (subclasscode == 0x3) {
            if (interface == 0x0)
               Pstdout("Generic RTC controller");
            else if (interface == 0x1)
               Pstdout("ISA RTC controller");
         } else
         if (subclasscode == 0x80 && interface == 0x0)
            Pstdout("Other system peripheral");
      } else 
      if (classcode == 0x9) {
         Pstdout("Input device - ");
         if ((subclasscode == 0x0) && (interface == 0x0))
            Pstdout("Keyboard controller");
         else if ((subclasscode == 0x1) && (interface == 0x0))
            Pstdout("Digitizer(pen)");
         else if ((subclasscode == 0x2) && (interface == 0x0))
            Pstdout("Mouse controller");
         else if ((subclasscode == 0x80) && (interface == 0x0))
            Pstdout("Other input controller");
      } else 
      if (classcode == 0xa) {
         Pstdout("docking station - ");
         if ((subclasscode == 0x0) && (interface == 0x0))
            Pstdout("Generic docking station");
         else if ((subclasscode == 0x80) && (interface == 0x0))
            Pstdout("Other type of docking station");
      } else 
      if (classcode == 0xb) {
         Pstdout("Processor - ");
         if ((subclasscode == 0x0) && (interface == 0x0))
            Pstdout("386");
         else if ((subclasscode == 0x1) && (interface == 0x0))
            Pstdout("486");
         else if ((subclasscode == 0x2) && (interface == 0x0))
            Pstdout("Pentium");
         else if ((subclasscode == 0x10) && (interface == 0x0))
            Pstdout("Alpha");
         else if ((subclasscode == 0x20) && (interface == 0x0))
            Pstdout("PowerPC");
         else if ((subclasscode == 0x40) && (interface == 0x0))
            Pstdout("Co-processor");
      } else 
      if (classcode == 0xc) {
         Pstdout("Serial Bus controller - ");
         if ((subclasscode == 0x0) && (interface == 0x0)) 
            Pstdout("Firewire(IEEE 1394)");
         else if ((subclasscode == 0x1) && (interface == 0x0))
            Pstdout("ACCESS.bus");
         else if ((subclasscode == 0x2) && (interface == 0x0))
            Pstdout("SSA(Serial Storage Architecture)");
         else if ((subclasscode == 0x3) && (interface == 0x0))
            Pstdout("USB(Universal Serial Bus)");
         else if ((subclasscode == 0x4) && (interface == 0x0))
            Pstdout("Fiber Channel(really copper!)");
      } else 
      if (classcode == 0xff) {
         Pstdout("device does not fit any of the defined classed codes");
      }
      Pstdout("\n");
      Pstdout("cache line size=0x%x\n",*((char *)p+12));
      Pstdout("latency timer=0x%x\n",*((char *)p+13));
      Pstdout("header type=0x%x\n",*((char *)p+14));
      Pstdout("BIST=0x%x\n",*((char *)p+15));
      Pstdout("Base Address 0=0x%x\n",p->base0);
      Pstdout("Base Address 1=0x%x\n",p->base1);
      Pstdout("Base Address 2=0x%x\n",p->base2);
      Pstdout("Base Address 3=0x%x\n",p->base3);
      Pstdout("Base Address 4=0x%x\n",p->base4);
      Pstdout("Base Address 5=0x%x\n",p->base5);
      Pstdout("CardBus CIS Pointer=0x%x\n",p->cardbus);
      Pstdout("Subsystem Vendor ID=0x%x\n",p->subsysvendorid);
      Pstdout("Subsystem ID=0x%x\n",p->subsystemid);
      Pstdout("Expansion ROM Base Address=0x%x\n",p->rombase);
      Pstdout("Interrupt line=0x%x\n",p->interruptline);
      Pstdout("Interrupt pin=0x%x\n",p->interruptpin);
      Pstdout("Min_Gnt=0x%x\n",p->min_gnt);
      Pstdout("Max_Lat=0x%x\n",p->max_lat);
      Pstdout("\n");
   }
   return(0);
}


/*
 *****************************************************************************
 * NOTE NOTE NOTE:
 * following code is from cmd.os/ca/ca.c
 * with the exit calls changed to error()
 * and a few bugs fixed.
 *****************************************************************************
 */

/* size of the data in eisa cmos */

#ifndef DMALLOC_FUNC_CHECK
/* extern int strlen(); */
extern int strncmp();
extern int strcmp();
#endif

static char data[EISA_BUFFER_SIZE];

static char *memory_type[] = {
   "System (base or extended)",
   "Expanded",
   "Virtual",
   "Other"
};

static char *memory_decode[] = {
   "20 Bits",
   "24 Bits",
   "32 Bits"
};


static char *width[] = {
   "Byte",
   "Word",
   "Double Word"
};

static char *dma_timing[] = {
   "ISA Compatible",
   "Type A",
   "Type B",
   "Type C (BURST)",
};


int 
ca_print_eisa(rm_key_t rmkey, u_int longoutput, u_int devconfig)
{
   char *bustypestr = NULL;
   int errcnt = 0;
   int bustype;
   char option;
   int slot = -1;
   struct cadata nvm;
   int cafd;
   int bflag = 0;    /* bustype flag */
   int sflag = 0;    /* slot flag */


   /* we don't use longoutput anywhere in this routine */

   if ((cafd = open(DEVCA, O_RDONLY)) == -1) {
      error(NOTBCFG,"ca_print_eisa: couldn't open %s, errno=%d",DEVCA,errno);
      return(-1);
   }

   nvm.ca_busaccess = devconfig;
   nvm.ca_buffer = data;
   nvm.ca_size = 0;

   if (ioctl(cafd, CA_EISA_READ_NVM, &nvm) != -1) {
      ca_eisa_parse_nvm(nvm.ca_busaccess, nvm.ca_buffer, nvm.ca_size);
   } else {
      error(NOTBCFG,"ioctl to read eisa config space failed");
      close(cafd);
      return(-1);
   }

   close(cafd);

   return(0);
}

int
ca_print_mca(rm_key_t rmkey, u_int longoutput, u_int devconfig)
{

   /* issue the CA_MCA_READ_POS ioctl when this actually does
    * something in the kernel and get rid of #if 0 in help[] array
    */
   Pstdout("Sorry, can't retrieve MCA information.\n");
   return(-1);  /* not written yet */

}

/* This section displays whatever is returned by the EISA CMOS Query. */

int
ca_eisa_parse_nvm(ulong_t slotnum, char *data, int length)
{
   int findex = 0;   /* function index number */
   NVM_SLOTINFO *slot = (NVM_SLOTINFO *)data;
   NVM_FUNCINFO *function;
#ifdef NOTYET
   eisa_funcinfo_t *ef;
#endif /* NOTYET */
   int c;

   Pstdout("\nSlot %d :\n", slotnum);

   while (slot < (NVM_SLOTINFO *)(data + length)) {
      Pstdout("\nBoard id : %c%c%c %x %x\n", 
         (slot->boardid[0] >> 2 &0x1f) + 64, 
         ((slot->boardid[0] << 3 | 
            slot->boardid[1] >> 5) & 0x1f) + 64, 
         (slot->boardid[1] &0x1f) + 64, 
         slot->boardid[2], 
         slot->boardid[3]);
      Pstdout("Revision : %x\n", slot->revision);
      Pstdout("Number of functions : %x\n", slot->functions);
      Pstdout("Function info : %x\n", slot->fib);
      Pstdout("Checksum : %x\n", slot->checksum);
      Pstdout("Duplicate id info : %x\n", slot->dupid);

      function = (NVM_FUNCINFO *)(slot + 1);
#ifdef NOTYET
      ef = (eisa_funcinfo_t *)(slot + 1);
#endif /* NOTYET */

      while (function < (NVM_FUNCINFO *)slot + slot->functions) {

         Pstdout("\n\tFunction number %d :\n", findex++);
#ifdef NOTYET 
         Pstdout("Type String = %s\n", (char *)ef->type);
#endif /* NOTYET */

         if (*(unsigned char *)&function->fib) {

             Pstdout("\n\tBoard id : %c%c%c %x %x\n", 
            (function->boardid[0] >> 2 &0x1f) + 64, 
            ((function->boardid[0] << 3 | 
                   function->boardid[1] >> 5) & 0x1f) + 64, 
            (function->boardid[1] & 0x1f) + 64, 
            function->boardid[2], 
            function->boardid[3]);
             Pstdout("\tDuplicate id info : %x\n", function->dupid);

             Pstdout("\tFunction info : %x\n", function->fib);
             if (function->fib.type)
            Pstdout("\tType;sub-type : %s\n", 
                     function->type);

             if (function->fib.data) {
            unsigned char *free_form_data = 
               function->u.freeform + 1;
            unsigned char *free_form_end = 
               free_form_data + *function->u.freeform;

            Pstdout("\tFree Form Data :\n\t");
            for (; free_form_data < free_form_end; free_form_data++)
               Pstdout("%u ", *free_form_data);
            Pstdout("\n");

             } else if (function->fib.memory) {
            NVM_MEMORY *memory = function->u.r.memory;

            Pstdout("\tMemory info :\n");
            while (memory < function->u.r.memory + NVM_MAX_MEMORY) {
                Pstdout("\t\tMemory Section %d:\n", 
               memory - function->u.r.memory + 1);
                Pstdout("\t\t\tLogical Characteristics:\n");
                if (memory->config.write)
               Pstdout("\t\t\t\tRead/Write\n");
                else
               Pstdout("\t\t\t\tRead Only\n");

                if (memory->config.cache)
               Pstdout("\t\t\t\tCached\n");

                Pstdout("\t\t\t\tType is %s\n", 
                         memory_type[memory->config.type]);
                if (memory->config.share)
               Pstdout("\t\t\t\tShared\n");

                Pstdout("\t\t\tPhysical Characteristics:\n");
                Pstdout("\t\t\t\tData Path Width: %s\n", 
               width[memory->datapath.width]);
                Pstdout("\t\t\t\tData Path Decode: %s\n", 
               memory_decode[memory->datapath.decode]);
                Pstdout("\t\t\tBoundaries:\n");
                Pstdout("\t\t\t\tStart address: %lx\n", 
               *(long *)memory->start * 256);
                Pstdout("\t\t\t\tSize: %lx\n", 
               (long)(memory->size * 1024));
                if (memory->config.more) 
               memory++;
                else 
               break;

            } /* end while */
             }

             if (function->fib.irq) {
            NVM_IRQ *irq = function->u.r.irq;

            Pstdout("\tIRQ info :\n");
            while (irq < function->u.r.irq + NVM_MAX_IRQ) {
                Pstdout("\t\tInterrupt Request Line: %d\n", irq->line);
#ifdef NOTYET 
         for (c = 0; c < EISA_MAX_IRQ; c++) {
            Pstdout("IRQ line: %d\n", ef->eisa_irq[c].line);
                if (ef->eisa_irq[c].trigger)
               Pstdout("\t\tLevel-triggerred\n");
                else
               Pstdout("\t\tEdge-triggerred\n");

                if (ef->eisa_irq[c].share)
               Pstdout("\t\tShareable\n");
         }
#endif /* NOTYET */
                if (irq->trigger)
               Pstdout("\t\tLevel-triggerred\n");
                else
               Pstdout("\t\tEdge-triggerred\n");

                if (irq->share)
               Pstdout("\t\tShareable\n");

                if (irq->more) 
               irq++;
                else 
               break;
            }   
             }

             if (function->fib.dma) {
            NVM_DMA *dma = function->u.r.dma;

            Pstdout("\tDMA info :\n");
            while (dma < function->u.r.dma + NVM_MAX_DMA) {
                Pstdout("\t\tDMA Device %d:\n", 
                  dma - function->u.r.dma + 1);
                Pstdout("\t\t\tChannel Number: %d\n", 
                  dma->channel);
                Pstdout("\t\t\tTransfer Size is %s\n", 
                  width[dma->width]);
                Pstdout("\t\t\tTransfer Timing is %s\n", 
                  dma_timing[dma->timing]);
                if (dma->share)
               Pstdout("\t\t\tShareable\n");

                if (dma->more) 
               dma++;
                else 
               break;
            }
             }

             if (function->fib.port) {
            NVM_PORT *port = function->u.r.port;

            Pstdout("\tPort info :\n");
            while (port < function->u.r.port + NVM_MAX_PORT) {
                Pstdout("\t\tPort Address: %x\n", 
                  port->address);
                Pstdout("\t\tSequential Ports: %d\n", 
                  port->count);
                if (port->share)
               Pstdout("\t\tShareable\n");

                if (port->more) 
               port++;
                else 
               break;
            }
             }

             if (function->fib.init) {
            unsigned char *init = function->u.r.init;

            Pstdout("\tInit info :\n\t");
            while (init < function->u.r.init + NVM_MAX_INIT)
                Pstdout("%u ", *init++);
            Pstdout("\n");
             }
         } /* end fib */

         function++;
#ifdef NOTYET
         ef++;
#endif /* NOTYET */
      }
      slot = (NVM_SLOTINFO *)function;
   }
}

#else   /* NETISL == 0 */

void
InitPCIVendor(void)
{
   return;
}

int 
ca_print_pci(rm_key_t rmkey, u_int longoutput, u_int devconfig)
{
   error(NOTBCFG,"no PCI CA commands in ndisl");
   return(-1);
}

int 
ca_print_eisa(rm_key_t rmkey, u_int longoutput, u_int devconfig)
{
   error(NOTBCFG,"no EISA CA commands in ndisl");
   return(-1);
}

int
ca_print_mca(rm_key_t rmkey, u_int longoutput, u_int devconfig)
{
   error(NOTBCFG,"no MCA CA commands in ndisl");
   return(-1);
}

#endif

int
showpcivendor(unsigned long vendorid)
{

   if (vendorid > 0xffff) {
      error(NOTBCFG,"bad vendor id %d",vendorid);
      return(-1);
   }

   StartList(2,"NAME",40);
   AddToList(1,pcivendor[vendorid]);
   EndList();
   return(0);
}
