/*
    OpenDCP: Builds Digital Cinema Packages
    Copyright (c) 2010-2013 Terrence Meiczinger, All Rights Reserved

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "opendcp.h"
#include "opendcp_image.h"

#define MAGIC_NUMBER 0x53445058
#define DEFAULT_GAMMA 1.0
#define FILM_GAMMA  0.6
#define VIDEO_GAMMA 1.7
#define DEFAULT_BLACK_POINT 95
#define DEFAULT_WHITE_POINT 685

typedef enum {
    DPX_DESCRIPTOR_RGB     = 50,
    DPX_DESCRIPTOR_RGBA    = 51,
    DPX_DESCRIPTOR_ABGR    = 52,
    DPX_DESCRIPTOR_YUV422  = 100,
    DPX_DESCRIPTOR_YUV4224 = 101,
    DPX_DESCRIPTOR_YUV444  = 102,
    DPX_DESCRIPTOR_YUV4444 = 103,
} dpx_descriptor_enum;

typedef enum {
    DPX_TRANSFER_USER               = 0,
    DPX_TRANSFER_PRINT_DENSITY      = 1,
    DPX_TRANSFER_LINEAR             = 2,
    DPX_TRANSFER_LOG                = 3,
    DPX_TRANSFER_UNSPECIFIED_VIDEO  = 4,
    DPX_TRANSFER_SMPTE240           = 5,
    DPX_TRANSFER_REC709             = 6,
    DPX_TRANSFER_REC601_625         = 7,
    DPX_TRANSFER_REC601_525         = 8,
    DPX_TRANSFER_NTSC               = 9,
    DPX_TRANSFER_PAL                = 10,
    DPX_TRANSFER_ZLINEAR            = 11,
    DPX_TRANSFER_ZHOMOGENOUS        = 12
} dpx_transfer_enum;

typedef enum {
    DPX_COLORIMETRIC_USER              = 0,
    DPX_COLORIMETRIC_PRINT_DENSITY     = 1,
    DPX_COLORIMETRIC_LINEAR            = 2,
    DPX_COLORIMETRIC_UNSPECIFIED_VIDEO = 4,
    DPX_COLORIMETRIC_SMPTE240          = 5,
    DPX_COLORIMETRIC_REC709            = 6,
    DPX_COLORIMETRIC_REC601_625        = 7,
    DPX_COLORIMETRIC_REC601_525        = 8,
    DPX_COLORIMETRIC_NTSC              = 9,
    DPX_COLORIMETRIC_PAL               = 10
} dpx_colorimetric_enum;

typedef enum
{
  DPX_PACKING_PACKED = 0,
  DPX_PACKING_LSB    = 1,
  DPX_PACKING_MSB    = 2
} dpx_packing_method_enum;

typedef struct {
    uint32_t magic_num;        /* magic number 0x53445058 (SDPX) or 0x58504453 (XPDS) */
    uint32_t offset;           /* offset to image data in bytes */
    char     version[8];       /* which header format version is being used (v1.0)*/
    uint32_t file_size;        /* file size in bytes */
    uint32_t ditto_key;        /* read time short cut - 0 = same, 1 = new */
    uint32_t gen_hdr_size;     /* generic header length in bytes */
    uint32_t ind_hdr_size;     /* industry header length in bytes */
    uint32_t user_data_size;   /* user-defined data length in bytes */
    char     file_name[100];   /* iamge file name */
    char     create_time[24];  /* file creation date "yyyy:mm:dd:hh:mm:ss:LTZ" */
    char     creator[100];     /* file creator's name */
    char     project[200];     /* project name */
    char     copyright[200];   /* right to use or copyright info */
    uint32_t key;              /* encryption ( FFFFFFFF = unencrypted ) */
    char     reserved[104];    /* reserved field TBD (need to pad) */
} dpx_file_header_t;

typedef struct {
    uint32_t    data_sign;         /* data sign (0 = unsigned, 1 = signed ) */
                                   /* "Core set images are unsigned" */
    uint32_t    ref_low_data;      /* reference low data code value */
    float       ref_low_quantity;  /* reference low quantity represented */
    uint32_t    ref_high_data;     /* reference high data code value */
    float       ref_high_quantity; /* reference high quantity represented */
    uint8_t     descriptor;        /* descriptor for image element */
    uint8_t     transfer;          /* transfer characteristics for element */
    uint8_t     colorimetric;      /* colormetric specification for element */
    uint8_t     bit_size;          /* bit size for element */
    uint16_t    packing;           /* packing for element */
    uint16_t    encoding;          /* encoding for element */
    uint32_t    data_offset;       /* offset to data of element */
    uint32_t    eol_padding;       /* end of line padding used in element */
    uint32_t    eo_image_padding;  /* end of image padding used in element */
    char        description[32];   /* description of element */
} dpx_image_element_t;

typedef struct {
    uint16_t    orientation;              /* image orientation */
    uint16_t    element_number;           /* number of image elements */
    uint32_t    pixels_per_line;          /* or x value */
    uint32_t    lines_per_image_ele;      /* or y value, per element */
    dpx_image_element_t image_element[8];
    char        reserved[52];             /* reserved for future use (padding) */
} dpx_image_header_t;

typedef struct {
    uint32_t    x_offset;               /* X offset */
    uint32_t    y_offset;               /* Y offset */
    int32_t     x_center;               /* X center */
    int32_t     y_center;               /* Y center */
    uint32_t    x_orig_size;            /* X original size */
    uint32_t    y_orig_size;            /* Y original size */
    char        file_name[100];         /* source image file name */
    char        creation_time[24];      /* source image creation date and time */
    char        input_dev[32];          /* input device name */
    char        input_serial[32];       /* input device serial number */
    uint16_t    border[4];              /* border validity (XL, XR, YT, YB) */
    uint32_t    pixel_aspect[2];        /* pixel aspect ratio (H:V) */
    uint8_t     reserved[28];           /* reserved for future use (padding) */
} dpx_image_orientation_t;

typedef struct {
    char        film_mfg_id[2];    /* film manufacturer ID code (2 digits from film edge code) */
    char        film_type[2];      /* file type (2 digits from film edge code) */
    char        offset[2];         /* offset in perfs (2 digits from film edge code)*/
    char        prefix[6];         /* prefix (6 digits from film edge code) */
    char        count[4];          /* count (4 digits from film edge code)*/
    char        format[32];        /* format (i.e. academy) */
    uint32_t    frame_position;    /* frame position in sequence */
    uint32_t    sequence_len;      /* sequence length in frames */
    uint32_t    held_count;        /* held count (1 = default) */
    float       frame_rate;        /* frame rate of original in frames/sec */
    float       shutter_angle;     /* shutter angle of camera in degrees */
    char        frame_id[32];      /* frame identification (i.e. keyframe) */
    char        slate_info[100];   /* slate information */
    char        reserved[56];      /* reserved for future use (padding) */
} dpx_film_header_t;

typedef struct {
    uint32_t    time_code;           /* SMPTE time code */
    uint32_t    user_bits;           /* SMPTE user bits */
    char        interlace;           /* interlace ( 0 = noninterlaced, 1 = 2:1 interlace*/
    char        field_num;           /* field number */
    char        video_signal;        /* video signal standard (table 4)*/
    char        unused;              /* used for byte alignment only */
    float       hor_sample_rate;     /* horizontal sampling rate in Hz */
    float       ver_sample_rate;     /* vertical sampling rate in Hz */
    float       frame_rate;          /* temporal sampling rate or frame rate in Hz */
    float       time_offset;         /* time offset from sync to first pixel */
    float       gamma;               /* gamma value */
    float       black_level;         /* black level code value */
    float       black_gain;          /* black gain */
    float       break_point;         /* breakpoint */
    float       white_level;         /* reference white level code value */
    float       integration_times;   /* integration time(s) */
    char        reserved[76];        /* reserved for future use (padding) */
} dpx_tv_header_t;

typedef struct {
    dpx_file_header_t        file;
    dpx_image_header_t       image;
    dpx_image_orientation_t  origin;
    dpx_film_header_t        film;
    dpx_tv_header_t          tv;
} dpx_image_t;

int lut[3][1024] = {
   /* Linear */
   { 0 },

   /* Log Film */
   {
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    1,    2,    4,    5,    7,    8,
       9,   11,   12,   14,   15,   17,   18,   20,   21,   22,   24,   25,   27,   28,   30,   32,   33,
      35,   36,   38,   39,   41,   42,   44,   46,   47,   49,   50,   52,   54,   55,   57,   59,   60,
      62,   64,   65,   67,   69,   70,   72,   74,   75,   77,   79,   81,   82,   84,   86,   88,   90,
      91,   93,   95,   97,   99,  100,  102,  104,  106,  108,  110,  112,  114,  115,  117,  119,  121,
     123,  125,  127,  129,  131,  133,  135,  137,  139,  141,  143,  145,  147,  149,  151,  153,  155,
     157,  160,  162,  164,  166,  168,  170,  172,  175,  177,  179,  181,  183,  186,  188,  190,  192,
     195,  197,  199,  201,  204,  206,  208,  211,  213,  215,  218,  220,  222,  225,  227,  230,  232,
     235,  237,  239,  242,  244,  247,  249,  252,  254,  257,  260,  262,  265,  267,  270,  273,  275,
     278,  280,  283,  286,  288,  291,  294,  297,  299,  302,  305,  308,  310,  313,  316,  319,  322,
     324,  327,  330,  333,  336,  339,  342,  345,  348,  351,  354,  357,  360,  363,  366,  369,  372,
     375,  378,  381,  384,  387,  390,  394,  397,  400,  403,  406,  410,  413,  416,  419,  423,  426,
     429,  433,  436,  439,  443,  446,  449,  453,  456,  460,  463,  467,  470,  474,  477,  481,  484,
     488,  492,  495,  499,  502,  506,  510,  514,  517,  521,  525,  528,  532,  536,  540,  544,  548,
     551,  555,  559,  563,  567,  571,  575,  579,  583,  587,  591,  595,  599,  603,  607,  612,  616,
     620,  624,  628,  633,  637,  641,  645,  650,  654,  658,  663,  667,  672,  676,  680,  685,  689,
     694,  698,  703,  708,  712,  717,  721,  726,  731,  735,  740,  745,  750,  754,  759,  764,  769,
     774,  779,  784,  788,  793,  798,  803,  808,  814,  819,  824,  829,  834,  839,  844,  849,  855,
     860,  865,  871,  876,  881,  887,  892,  897,  903,  908,  914,  919,  925,  931,  936,  942,  947,
     953,  959,  965,  970,  976,  982,  988,  994,  999, 1005, 1011, 1017, 1023, 1029, 1035, 1041, 1048,
    1054, 1060, 1066, 1072, 1078, 1085, 1091, 1097, 1104, 1110, 1116, 1123, 1129, 1136, 1142, 1149, 1156,
    1162, 1169, 1176, 1182, 1189, 1196, 1203, 1209, 1216, 1223, 1230, 1237, 1244, 1251, 1258, 1265, 1272,
    1279, 1287, 1294, 1301, 1308, 1316, 1323, 1330, 1338, 1345, 1353, 1360, 1368, 1375, 1383, 1391, 1398,
    1406, 1414, 1422, 1429, 1437, 1445, 1453, 1461, 1469, 1477, 1485, 1493, 1501, 1510, 1518, 1526, 1534,
    1543, 1551, 1559, 1568, 1576, 1585, 1593, 1602, 1611, 1619, 1628, 1637, 1646, 1655, 1663, 1672, 1681,
    1690, 1699, 1708, 1718, 1727, 1736, 1745, 1754, 1764, 1773, 1783, 1792, 1801, 1811, 1821, 1830, 1840,
    1850, 1859, 1869, 1879, 1889, 1899, 1909, 1919, 1929, 1939, 1949, 1959, 1970, 1980, 1990, 2001, 2011,
    2022, 2032, 2043, 2053, 2064, 2075, 2086, 2096, 2107, 2118, 2129, 2140, 2151, 2162, 2174, 2185, 2196,
    2207, 2219, 2230, 2242, 2253, 2265, 2277, 2288, 2300, 2312, 2324, 2336, 2347, 2359, 2372, 2384, 2396,
    2408, 2420, 2433, 2445, 2458, 2470, 2483, 2495, 2508, 2521, 2533, 2546, 2559, 2572, 2585, 2598, 2611,
    2625, 2638, 2651, 2665, 2678, 2692, 2705, 2719, 2733, 2746, 2760, 2774, 2788, 2802, 2816, 2830, 2844,
    2859, 2873, 2887, 2902, 2916, 2931, 2946, 2960, 2975, 2990, 3005, 3020, 3035, 3050, 3065, 3080, 3096,
    3111, 3127, 3142, 3158, 3173, 3189, 3205, 3221, 3237, 3253, 3269, 3285, 3301, 3318, 3334, 3351, 3367,
    3384, 3401, 3417, 3434, 3451, 3468, 3485, 3502, 3520, 3537, 3554, 3572, 3589, 3607, 3625, 3642, 3660,
    3678, 3696, 3714, 3733, 3751, 3769, 3788, 3806, 3825, 3844, 3862, 3881, 3900, 3919, 3938, 3958, 3977,
    3996, 4016, 4035, 4055, 4075, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095 },

    /* Log Video */
    {
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
       0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    4,    8,   12,   16,   20,   25,
      29,   33,   37,   42,   46,   50,   54,   59,   63,   67,   72,   76,   80,   84,   89,   93,   97,
     102,  106,  111,  115,  119,  124,  128,  132,  137,  141,  146,  150,  154,  159,  163,  168,  172,
     177,  181,  186,  190,  195,  199,  204,  208,  213,  217,  222,  226,  231,  235,  240,  244,  249,
     254,  258,  263,  267,  272,  277,  281,  286,  291,  295,  300,  305,  309,  314,  319,  323,  328,
     333,  337,  342,  347,  352,  356,  361,  366,  371,  375,  380,  385,  390,  395,  399,  404,  409,
     414,  419,  424,  428,  433,  438,  443,  448,  453,  458,  463,  468,  473,  477,  482,  487,  492,
     497,  502,  507,  512,  517,  522,  527,  532,  537,  542,  547,  553,  558,  563,  568,  573,  578,
     583,  588,  593,  598,  604,  609,  614,  619,  624,  629,  635,  640,  645,  650,  655,  661,  666,
     671,  676,  682,  687,  692,  697,  703,  708,  713,  719,  724,  729,  735,  740,  745,  751,  756,
     762,  767,  772,  778,  783,  789,  794,  800,  805,  811,  816,  822,  827,  833,  838,  844,  849,
     855,  860,  866,  871,  877,  882,  888,  894,  899,  905,  911,  916,  922,  927,  933,  939,  944,
     950,  956,  962,  967,  973,  979,  985,  990,  996, 1002, 1008, 1013, 1019, 1025, 1031, 1037, 1042,
    1048, 1054, 1060, 1066, 1072, 1078, 1084, 1090, 1095, 1101, 1107, 1113, 1119, 1125, 1131, 1137, 1143,
    1149, 1155, 1161, 1167, 1173, 1179, 1185, 1192, 1198, 1204, 1210, 1216, 1222, 1228, 1234, 1240, 1247,
    1253, 1259, 1265, 1271, 1278, 1284, 1290, 1296, 1303, 1309, 1315, 1321, 1328, 1334, 1340, 1347, 1353,
    1359, 1366, 1372, 1378, 1385, 1391, 1398, 1404, 1410, 1417, 1423, 1430, 1436, 1443, 1449, 1456, 1462,
    1469, 1475, 1482, 1488, 1495, 1501, 1508, 1515, 1521, 1528, 1534, 1541, 1548, 1554, 1561, 1568, 1574,
    1581, 1588, 1595, 1601, 1608, 1615, 1622, 1628, 1635, 1642, 1649, 1655, 1662, 1669, 1676, 1683, 1690,
    1697, 1704, 1710, 1717, 1724, 1731, 1738, 1745, 1752, 1759, 1766, 1773, 1780, 1787, 1794, 1801, 1808,
    1815, 1822, 1829, 1837, 1844, 1851, 1858, 1865, 1872, 1879, 1887, 1894, 1901, 1908, 1915, 1923, 1930,
    1937, 1944, 1952, 1959, 1966, 1974, 1981, 1988, 1996, 2003, 2010, 2018, 2025, 2033, 2040, 2048, 2055,
    2062, 2070, 2077, 2085, 2092, 2100, 2108, 2115, 2123, 2130, 2138, 2145, 2153, 2161, 2168, 2176, 2184,
    2191, 2199, 2207, 2214, 2222, 2230, 2237, 2245, 2253, 2261, 2269, 2276, 2284, 2292, 2300, 2308, 2316,
    2323, 2331, 2339, 2347, 2355, 2363, 2371, 2379, 2387, 2395, 2403, 2411, 2419, 2427, 2435, 2443, 2451,
    2459, 2467, 2476, 2484, 2492, 2500, 2508, 2516, 2525, 2533, 2541, 2549, 2557, 2566, 2574, 2582, 2591,
    2599, 2607, 2616, 2624, 2632, 2641, 2649, 2658, 2666, 2674, 2683, 2691, 2700, 2708, 2717, 2725, 2734,
    2742, 2751, 2760, 2768, 2777, 2785, 2794, 2803, 2811, 2820, 2829, 2837, 2846, 2855, 2863, 2872, 2881,
    2890, 2899, 2907, 2916, 2925, 2934, 2943, 2952, 2960, 2969, 2978, 2987, 2996, 3005, 3014, 3023, 3032,
    3041, 3050, 3059, 3068, 3077, 3087, 3096, 3105, 3114, 3123, 3132, 3141, 3151, 3160, 3169, 3178, 3187,
    3197, 3206, 3215, 3225, 3234, 3243, 3253, 3262, 3271, 3281, 3290, 3300, 3309, 3319, 3328, 3338, 3347,
    3357, 3366, 3376, 3385, 3395, 3404, 3414, 3424, 3433, 3443, 3453, 3462, 3472, 3482, 3492, 3501, 3511,
    3521, 3531, 3540, 3550, 3560, 3570, 3580, 3590, 3600, 3610, 3620, 3630, 3640, 3650, 3660, 3670, 3680,
    3690, 3700, 3710, 3720, 3730, 3740, 3750, 3760, 3771, 3781, 3791, 3801, 3812, 3822, 3832, 3842, 3853,
    3863, 3873, 3884, 3894, 3905, 3915, 3925, 3936, 3946, 3957, 3967, 3978, 3988, 3999, 4009, 4020, 4031,
    4041, 4052, 4062, 4073, 4084, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095, 4095,
    4095, 4095, 4095, 4095 }
};

static inline uint8_t  r_8(uint8_t value, int endian) {
    if (endian) {
        return (value << 8) | (value >> 8);
    } else {
        return value;
    }
}

static inline uint16_t r_16(uint16_t value, int endian) {
    if (endian) {
        return (uint16_t)((value & 0xFFU) << 8 | (value & 0xFF00U) >> 8);
    } else {
        return value;
    }
}

static inline uint32_t r_32(uint32_t value, int endian) {
    if (endian) {
        return (value & 0x000000FFU) << 24 | (value & 0x0000FF00U) << 8 |
               (value & 0x00FF0000U) >> 8 | (value & 0xFF000000U) >> 24;
    } else {
        return value;
    }
}

static double dpx_log(int in, int white, float gamma) {
    return (pow(10.0,((in - white) * 0.002 / gamma * 1.0/1.7)));
}

int dpx_log_to_lin(int value, float gamma) {
    double gain;
    double offset;

    gain = 4095.0 / (1 - dpx_log(DEFAULT_BLACK_POINT, DEFAULT_WHITE_POINT, gamma));
    offset = gain - 4095;

    if (value <= DEFAULT_BLACK_POINT) {
        return 0;
    }

    if (value < DEFAULT_WHITE_POINT) {
        double f_i;
        f_i = dpx_log(value, DEFAULT_WHITE_POINT, gamma);
        return ((int)((f_i*gain)-offset));
    }

    return 4095;
}

void buildLut() {
    int x;

    for (x=0;x<1024;x++) {
       dpx_log_to_lin(x, 1.7);
    }
}

void print_dpx_header(dpx_image_t *dpx, int endian) {
    OPENDCP_LOG(LOG_DEBUG,"magic num %d",r_32(dpx->file.magic_num,endian));
    OPENDCP_LOG(LOG_DEBUG,"offset %d",r_32(dpx->file.offset,endian));
    OPENDCP_LOG(LOG_DEBUG,"version %s",dpx->file.version);
    OPENDCP_LOG(LOG_DEBUG,"file size %d",r_32(dpx->file.file_size,endian));
    OPENDCP_LOG(LOG_DEBUG,"file name %s",dpx->file.file_name);
    OPENDCP_LOG(LOG_DEBUG,"creator %s",dpx->file.creator);

    OPENDCP_LOG(LOG_DEBUG,"orientation %d",r_16(dpx->image.orientation,endian));
    OPENDCP_LOG(LOG_DEBUG,"elem number %d",r_16(dpx->image.element_number,endian));
    OPENDCP_LOG(LOG_DEBUG,"pixels_per_line %d",r_32(dpx->image.pixels_per_line,endian));
    OPENDCP_LOG(LOG_DEBUG,"line per elem %d",r_32(dpx->image.lines_per_image_ele,endian));
    OPENDCP_LOG(LOG_DEBUG,"descriptor %d",dpx->image.image_element[0].descriptor);
    OPENDCP_LOG(LOG_DEBUG,"transfer %d",dpx->image.image_element[0].transfer);
    OPENDCP_LOG(LOG_DEBUG,"colorimetric %d",dpx->image.image_element[0].colorimetric);
    OPENDCP_LOG(LOG_DEBUG,"bit size: %d",dpx->image.image_element[0].bit_size);
    OPENDCP_LOG(LOG_DEBUG,"packing: %d",r_16(dpx->image.image_element[0].packing,endian));
    OPENDCP_LOG(LOG_DEBUG,"encoding: %d",r_16(dpx->image.image_element[0].encoding,endian));
    OPENDCP_LOG(LOG_DEBUG,"data offset: %d",r_32(dpx->image.image_element[0].data_offset,endian));
}

/*!
 @function opendcp_decode_dpx
 @abstract Read an image file and populates an opendcp_image_t structure.
 @discussion This function will read and decode a file and place the
     decoded image in an opendcp_image_t struct.
 @param image_ptr Pointer to the destination opendcp_image_t struct.
 @param sfile The name if the source image file.
 @return OPENDCP_ERROR value
*/
int opendcp_decode_dpx(opendcp_image_t **image_ptr, const char *sfile) {
    dpx_image_t     dpx;
    FILE            *dpx_fp;
    opendcp_image_t    *image = 00;
    int image_size,endian,logarithmic = 0;
    int i,j,w,h,bps,spp;

    OPENDCP_LOG(LOG_DEBUG,"DPX decode begin");

    /* FIX: add dpx loogarithmic  option */
    int dpx_log = 0;

    /* open dpx using filename or file descriptor */
    dpx_fp = fopen(sfile, "rb");

    if (!dpx_fp) {
        OPENDCP_LOG(LOG_ERROR,"Failed to open %s for reading", sfile);
        return OPENDCP_ERROR;
    }

    fread(&dpx,sizeof(dpx_image_t),1,dpx_fp);

    if (dpx.file.magic_num == MAGIC_NUMBER) {
        endian = 0;
    } else if (r_32(dpx.file.magic_num, 1) == MAGIC_NUMBER) {
        endian = 1;
    } else {
         OPENDCP_LOG(LOG_ERROR,"%s is not a valid DPX file", sfile);
         return OPENDCP_ERROR;
    }

    bps = dpx.image.image_element[0].bit_size;

    if (bps < 8 || bps > 16) {
        OPENDCP_LOG(LOG_ERROR, "%d-bit depth is not supported\n",bps);
        return OPENDCP_ERROR;
    }

    switch (dpx.image.image_element[0].descriptor) {
        case DPX_DESCRIPTOR_RGB:
            spp = 3;
            break;
        case DPX_DESCRIPTOR_RGBA:
            spp = 4;
            break;
        case DPX_DESCRIPTOR_YUV422:
            spp = 2;
            break;
        case DPX_DESCRIPTOR_YUV4224:
            spp = 3;
            break;
        case DPX_DESCRIPTOR_YUV444:
            spp = 3;
            break;
        case DPX_DESCRIPTOR_YUV4444:
            spp = 4;
            break;
        default:
            OPENDCP_LOG(LOG_ERROR, "Unsupported image descriptor: %d\n", dpx.image.image_element[0].descriptor);
            return OPENDCP_ERROR;
            break;
    }

    print_dpx_header(&dpx,endian);

    if (dpx_log == DPX_LINEAR) {
        logarithmic = 0;
    } else if (dpx_log == DPX_FILM) {
        logarithmic = 1;
    } else if (dpx_log == DPX_VIDEO) {
        logarithmic = 1;
    }

    w = r_32(dpx.image.pixels_per_line, endian);
    h = r_32(dpx.image.lines_per_image_ele, endian);

    image_size = w * h;

    /* create the image */
    image = opendcp_image_create(3,w,h);

    fseek(dpx_fp, r_32(dpx.file.offset, endian), SEEK_SET);

    /* YUV422 */
    if (dpx.image.image_element[0].descriptor == DPX_DESCRIPTOR_YUV422) {
        /* 8 bits per pixel */
        if (bps == 8) {
            uint8_t data[4];
            rgb_pixel_float_t p;
            for (i=0; i<image_size; i+=2) {
                fread(&data,sizeof(data),1,dpx_fp);
                for (j=0; j<spp; j++) {
                    p = yuv444toRGB888(data[1+(2*j)], data[0], data[2]);
                    image->component[0].data[i+j] = lut[dpx_log][((int)p.r << 2)];
                    image->component[1].data[i+j] = lut[dpx_log][((int)p.g << 2)];
                    image->component[2].data[i+j] = lut[dpx_log][((int)p.b << 2)];
                }
            }
        }
    }
    /* YUV422 */

    /* RGB(A) */
    if (dpx.image.image_element[0].descriptor == DPX_DESCRIPTOR_RGB || dpx.image.image_element[0].descriptor == DPX_DESCRIPTOR_RGBA) {
        /* 8 bits per pixel */
        if (bps == 8) {
            uint8_t  data;
            for (i=0; i<image_size; i++) {
                for (j=0; j<spp; j++) {
                    data = fgetc(dpx_fp);
                    if (j < 3) { // Skip alpha channel
                        image->component[j].data[i] = data << 4;
                    }
                }
            }
        /* 10 bits per pixel */
        } else if (bps == 10) {
            uint32_t data;
            uint32_t comp;
            for (i=0; i<image_size; i++) {
                fread(&data,sizeof(data),1,dpx_fp);
                for (j=0; j<spp; j++) {
                    if (j==0) {
                        comp = r_32(data, endian) >> 16;
                        image->component[j].data[i] = ((comp & 0xFFF0) >> 4) | ((comp & 0x00CF) >> 6);
                    } else if (j==1) {
                        comp = r_32(data, endian) >> 6;
                        image->component[j].data[i] = ((comp & 0xFFF0) >> 4) | ((comp & 0x00CF) >> 6);
                    } else if (j==2) {
                        comp = r_32(data, endian) << 4;
                        image->component[j].data[i] = ((comp & 0xFFF0) >> 4) | ((comp & 0x00CF) >> 6);
                    }
                    if (logarithmic) {
                        image->component[j].data[i] = lut[dpx_log][(image->component[j].data[i] >> 2)];
                    }
                }
            }
        /* 12 bits per pixel */
        } else if (bps == 12) {
            uint8_t  data[2];
            for (i=0; i<image_size; i++) {
                for (j=0; j<spp; j++) {
                    data[0] = fgetc(dpx_fp);
                    data[1] = fgetc(dpx_fp);
                    if (j < 3) {
                        image->component[j].data[i] = (data[!endian]<<4) | (data[endian]>>4);
                    }
                }
            }
        /* 16 bits per pixel */
        } else if ( bps == 16) {
            uint8_t  data[2];
            for (i=0; i<image_size; i++) {
                for (j=0; j<spp; j++) {
                    data[0] = fgetc(dpx_fp);
                    data[1] = fgetc(dpx_fp);
                    if (j < 3) { // Skip alpha channel
                        image->component[j].data[i] = ( data[!endian] << 8 ) | data[endian];
                        image->component[j].data[i] = (image->component[j].data[i]) >> 4;
                    }
                }
            }
        }
    }
    /* RGB(A) */

    fclose(dpx_fp);

    OPENDCP_LOG(LOG_DEBUG,"DPX decode done");
    *image_ptr = image;

    return OPENDCP_NO_ERROR;
}
