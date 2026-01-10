#include "cairo_device_internal.h"

//
// Mapping table for character names to Unicode values.
//

typedef struct name_map_s
{
  const char	*name;			// Character name
  int		unicode;		// Unicode value
} name_map_t;

static name_map_t	unicode_map[] =
{
  { "A",		0x0041 },
  { "AE",		0x00c6 },
  { "AEacute",		0x01fc },
  { "AEsmall",		0xf7e6 },
  { "Aacute",		0x00c1 },
  { "Aacutesmall",	0xf7e1 },
  { "Abreve",		0x0102 },
  { "Acircumflex",	0x00c2 },
  { "Acircumflexsmall",	0xf7e2 },
  { "Acute",		0xf6c9 },
  { "Acutesmall",	0xf7b4 },
  { "Adieresis",	0x00c4 },
  { "Adieresissmall",	0xf7e4 },
  { "Agrave",		0x00c0 },
  { "Agravesmall",	0xf7e0 },
  { "Alpha",		0x0391 },
  { "Alphatonos",	0x0386 },
  { "Amacron",		0x0100 },
  { "Aogonek",		0x0104 },
  { "Aring",		0x00c5 },
  { "Aringacute",	0x01fa },
  { "Aringsmall",	0xf7e5 },
  { "Asmall",		0xf761 },
  { "Atilde",		0x00c3 },
  { "Atildesmall",	0xf7e3 },
  { "B",		0x0042 },
  { "Beta",		0x0392 },
  { "Brevesmall",	0xf6f4 },
  { "Bsmall",		0xf762 },
  { "C",		0x0043 },
  { "Cacute",		0x0106 },
  { "Caron",		0xf6ca },
  { "Caronsmall",	0xf6f5 },
  { "Ccaron",		0x010c },
  { "Ccedilla",		0x00c7 },
  { "Ccedillasmall",	0xf7e7 },
  { "Ccircumflex",	0x0108 },
  { "Cdotaccent",	0x010a },
  { "Cedillasmall",	0xf7b8 },
  { "Chi",		0x03a7 },
  { "Circumflexsmall",	0xf6f6 },
  { "Csmall",		0xf763 },
  { "D",		0x0044 },
  { "Dcaron",		0x010e },
  { "Dcroat",		0x0110 },
  { "Delta",		0x0394 },
  { "Delta",		0x2206 },
  { "Dieresis",		0xf6cb },
  { "DieresisAcute",	0xf6cc },
  { "DieresisGrave",	0xf6cd },
  { "Dieresissmall",	0xf7a8 },
  { "Dotaccentsmall",	0xf6f7 },
  { "Dsmall",		0xf764 },
  { "E",		0x0045 },
  { "Eacute",		0x00c9 },
  { "Eacutesmall",	0xf7e9 },
  { "Ebreve",		0x0114 },
  { "Ecaron",		0x011a },
  { "Ecircumflex",	0x00ca },
  { "Ecircumflexsmall",	0xf7ea },
  { "Edieresis",	0x00cb },
  { "Edieresissmall",	0xf7eb },
  { "Edotaccent",	0x0116 },
  { "Egrave",		0x00c8 },
  { "Egravesmall",	0xf7e8 },
  { "Emacron",		0x0112 },
  { "Eng",		0x014a },
  { "Eogonek",		0x0118 },
  { "Epsilon",		0x0395 },
  { "Epsilontonos",	0x0388 },
  { "Esmall",		0xf765 },
  { "Eta",		0x0397 },
  { "Etatonos",		0x0389 },
  { "Eth",		0x00d0 },
  { "Ethsmall",		0xf7f0 },
  { "Euro",		0x20ac },
  { "F",		0x0046 },
  { "Fsmall",		0xf766 },
  { "G",		0x0047 },
  { "Gamma",		0x0393 },
  { "Gbreve",		0x011e },
  { "Gcaron",		0x01e6 },
  { "Gcircumflex",	0x011c },
  { "Gcommaaccent",	0x0122 },
  { "Gdotaccent",	0x0120 },
  { "Grave",		0xf6ce },
  { "Gravesmall",	0xf760 },
  { "Gsmall",		0xf767 },
  { "H",		0x0048 },
  { "H18533",		0x25cf },
  { "H18543",		0x25aa },
  { "H18551",		0x25ab },
  { "H22073",		0x25a1 },
  { "Hbar",		0x0126 },
  { "Hcircumflex",	0x0124 },
  { "Hsmall",		0xf768 },
  { "Hungarumlaut",	0xf6cf },
  { "Hungarumlautsmall", 0xf6f8 },
  { "I",		0x0049 },
  { "IJ",		0x0132 },
  { "Iacute",		0x00cd },
  { "Iacutesmall",	0xf7ed },
  { "Ibreve",		0x012c },
  { "Icircumflex",	0x00ce },
  { "Icircumflexsmall",	0xf7ee },
  { "Idieresis",	0x00cf },
  { "Idieresissmall",	0xf7ef },
  { "Idotaccent",	0x0130 },
  { "Ifraktur",		0x2111 },
  { "Igrave",		0x00cc },
  { "Igravesmall",	0xf7ec },
  { "Imacron",		0x012a },
  { "Iogonek",		0x012e },
  { "Iota",		0x0399 },
  { "Iotadieresis",	0x03aa },
  { "Iotatonos",	0x038a },
  { "Ismall",		0xf769 },
  { "Itilde",		0x0128 },
  { "J",		0x004a },
  { "Jcircumflex",	0x0134 },
  { "Jsmall",		0xf76a },
  { "K",		0x004b },
  { "Kappa",		0x039a },
  { "Kcommaaccent",	0x0136 },
  { "Ksmall",		0xf76b },
  { "L",		0x004c },
  { "LL",		0xf6bf },
  { "Lacute",		0x0139 },
  { "Lambda",		0x039b },
  { "Lcaron",		0x013d },
  { "Lcommaaccent",	0x013b },
  { "Ldot",		0x013f },
  { "Lslash",		0x0141 },
  { "Lslashsmall",	0xf6f9 },
  { "Lsmall",		0xf76c },
  { "M",		0x004d },
  { "Macron",		0xf6d0 },
  { "Macronsmall",	0xf7af },
  { "Msmall",		0xf76d },
  { "Mu",		0x039c },
  { "N",		0x004e },
  { "Nacute",		0x0143 },
  { "Ncaron",		0x0147 },
  { "Ncommaaccent",	0x0145 },
  { "Nsmall",		0xf76e },
  { "Ntilde",		0x00d1 },
  { "Ntildesmall",	0xf7f1 },
  { "Nu",		0x039d },
  { "O",		0x004f },
  { "OE",		0x0152 },
  { "OEsmall",		0xf6fa },
  { "Oacute",		0x00d3 },
  { "Oacutesmall",	0xf7f3 },
  { "Obreve",		0x014e },
  { "Ocircumflex",	0x00d4 },
  { "Ocircumflexsmall",	0xf7f4 },
  { "Odieresis",	0x00d6 },
  { "Odieresissmall",	0xf7f6 },
  { "Ogoneksmall",	0xf6fb },
  { "Ograve",		0x00d2 },
  { "Ogravesmall",	0xf7f2 },
  { "Ohorn",		0x01a0 },
  { "Ohungarumlaut",	0x0150 },
  { "Omacron",		0x014c },
  { "Omega",		0x03a9 },
  { "Omega",		0x2126 },
  { "Omegatonos",	0x038f },
  { "Omicron",		0x039f },
  { "Omicrontonos",	0x038c },
  { "Oslash",		0x00d8 },
  { "Oslashacute",	0x01fe },
  { "Oslashsmall",	0xf7f8 },
  { "Osmall",		0xf76f },
  { "Otilde",		0x00d5 },
  { "Otildesmall",	0xf7f5 },
  { "P",		0x0050 },
  { "Phi",		0x03a6 },
  { "Pi",		0x03a0 },
  { "Psi",		0x03a8 },
  { "Psmall",		0xf770 },
  { "Q",		0x0051 },
  { "Qsmall",		0xf771 },
  { "R",		0x0052 },
  { "Racute",		0x0154 },
  { "Rcaron",		0x0158 },
  { "Rcommaaccent",	0x0156 },
  { "Rfraktur",		0x211c },
  { "Rho",		0x03a1 },
  { "Ringsmall",	0xf6fc },
  { "Rsmall",		0xf772 },
  { "S",		0x0053 },
  { "SF010000",		0x250c },
  { "SF020000",		0x2514 },
  { "SF030000",		0x2510 },
  { "SF040000",		0x2518 },
  { "SF050000",		0x253c },
  { "SF060000",		0x252c },
  { "SF070000",		0x2534 },
  { "SF080000",		0x251c },
  { "SF090000",		0x2524 },
  { "SF100000",		0x2500 },
  { "SF110000",		0x2502 },
  { "SF190000",		0x2561 },
  { "SF200000",		0x2562 },
  { "SF210000",		0x2556 },
  { "SF220000",		0x2555 },
  { "SF230000",		0x2563 },
  { "SF240000",		0x2551 },
  { "SF250000",		0x2557 },
  { "SF260000",		0x255d },
  { "SF270000",		0x255c },
  { "SF280000",		0x255b },
  { "SF360000",		0x255e },
  { "SF370000",		0x255f },
  { "SF380000",		0x255a },
  { "SF390000",		0x2554 },
  { "SF400000",		0x2569 },
  { "SF410000",		0x2566 },
  { "SF420000",		0x2560 },
  { "SF430000",		0x2550 },
  { "SF440000",		0x256c },
  { "SF450000",		0x2567 },
  { "SF460000",		0x2568 },
  { "SF470000",		0x2564 },
  { "SF480000",		0x2565 },
  { "SF490000",		0x2559 },
  { "SF500000",		0x2558 },
  { "SF510000",		0x2552 },
  { "SF520000",		0x2553 },
  { "SF530000",		0x256b },
  { "SF540000",		0x256a },
  { "Sacute",		0x015a },
  { "Scaron",		0x0160 },
  { "Scaronsmall",	0xf6fd },
  { "Scedilla",		0x015e },
  { "Scedilla",		0xf6c1 },
  { "Scircumflex",	0x015c },
  { "Scommaaccent",	0x0218 },
  { "Sigma",		0x03a3 },
  { "Ssmall",		0xf773 },
  { "T",		0x0054 },
  { "Tau",		0x03a4 },
  { "Tbar",		0x0166 },
  { "Tcaron",		0x0164 },
  { "Tcommaaccent",	0x0162 },
  { "Tcommaaccent",	0x021a },
  { "Theta",		0x0398 },
  { "Thorn",		0x00de },
  { "Thornsmall",	0xf7fe },
  { "Tildesmall",	0xf6fe },
  { "Tsmall",		0xf774 },
  { "U",		0x0055 },
  { "Uacute",		0x00da },
  { "Uacutesmall",	0xf7fa },
  { "Ubreve",		0x016c },
  { "Ucircumflex",	0x00db },
  { "Ucircumflexsmall",	0xf7fb },
  { "Udieresis",	0x00dc },
  { "Udieresissmall",	0xf7fc },
  { "Ugrave",		0x00d9 },
  { "Ugravesmall",	0xf7f9 },
  { "Uhorn",		0x01af },
  { "Uhungarumlaut",	0x0170 },
  { "Umacron",		0x016a },
  { "Uogonek",		0x0172 },
  { "Upsilon",		0x03a5 },
  { "Upsilon1",		0x03d2 },
  { "Upsilondieresis",	0x03ab },
  { "Upsilontonos",	0x038e },
  { "Uring",		0x016e },
  { "Usmall",		0xf775 },
  { "Utilde",		0x0168 },
  { "V",		0x0056 },
  { "Vsmall",		0xf776 },
  { "W",		0x0057 },
  { "Wacute",		0x1e82 },
  { "Wcircumflex",	0x0174 },
  { "Wdieresis",	0x1e84 },
  { "Wgrave",		0x1e80 },
  { "Wsmall",		0xf777 },
  { "X",		0x0058 },
  { "Xi",		0x039e },
  { "Xsmall",		0xf778 },
  { "Y",		0x0059 },
  { "Yacute",		0x00dd },
  { "Yacutesmall",	0xf7fd },
  { "Ycircumflex",	0x0176 },
  { "Ydieresis",	0x0178 },
  { "Ydieresissmall",	0xf7ff },
  { "Ygrave",		0x1ef2 },
  { "Ysmall",		0xf779 },
  { "Z",		0x005a },
  { "Zacute",		0x0179 },
  { "Zcaron",		0x017d },
  { "Zcaronsmall",	0xf6ff },
  { "Zdotaccent",	0x017b },
  { "Zeta",		0x0396 },
  { "Zsmall",		0xf77a },
  { "a",		0x0061 },
  { "aacute",		0x00e1 },
  { "abreve",		0x0103 },
  { "acircumflex",	0x00e2 },
  { "acute",		0x00b4 },
  { "acutecomb",	0x0301 },
  { "adieresis",	0x00e4 },
  { "ae",		0x00e6 },
  { "aeacute",		0x01fd },
  { "afii00208",	0x2015 },
  { "afii10017",	0x0410 },
  { "afii10018",	0x0411 },
  { "afii10019",	0x0412 },
  { "afii10020",	0x0413 },
  { "afii10021",	0x0414 },
  { "afii10022",	0x0415 },
  { "afii10023",	0x0401 },
  { "afii10024",	0x0416 },
  { "afii10025",	0x0417 },
  { "afii10026",	0x0418 },
  { "afii10027",	0x0419 },
  { "afii10028",	0x041a },
  { "afii10029",	0x041b },
  { "afii10030",	0x041c },
  { "afii10031",	0x041d },
  { "afii10032",	0x041e },
  { "afii10033",	0x041f },
  { "afii10034",	0x0420 },
  { "afii10035",	0x0421 },
  { "afii10036",	0x0422 },
  { "afii10037",	0x0423 },
  { "afii10038",	0x0424 },
  { "afii10039",	0x0425 },
  { "afii10040",	0x0426 },
  { "afii10041",	0x0427 },
  { "afii10042",	0x0428 },
  { "afii10043",	0x0429 },
  { "afii10044",	0x042a },
  { "afii10045",	0x042b },
  { "afii10046",	0x042c },
  { "afii10047",	0x042d },
  { "afii10048",	0x042e },
  { "afii10049",	0x042f },
  { "afii10050",	0x0490 },
  { "afii10051",	0x0402 },
  { "afii10052",	0x0403 },
  { "afii10053",	0x0404 },
  { "afii10054",	0x0405 },
  { "afii10055",	0x0406 },
  { "afii10056",	0x0407 },
  { "afii10057",	0x0408 },
  { "afii10058",	0x0409 },
  { "afii10059",	0x040a },
  { "afii10060",	0x040b },
  { "afii10061",	0x040c },
  { "afii10062",	0x040e },
  { "afii10063",	0xf6c4 },
  { "afii10064",	0xf6c5 },
  { "afii10065",	0x0430 },
  { "afii10066",	0x0431 },
  { "afii10067",	0x0432 },
  { "afii10068",	0x0433 },
  { "afii10069",	0x0434 },
  { "afii10070",	0x0435 },
  { "afii10071",	0x0451 },
  { "afii10072",	0x0436 },
  { "afii10073",	0x0437 },
  { "afii10074",	0x0438 },
  { "afii10075",	0x0439 },
  { "afii10076",	0x043a },
  { "afii10077",	0x043b },
  { "afii10078",	0x043c },
  { "afii10079",	0x043d },
  { "afii10080",	0x043e },
  { "afii10081",	0x043f },
  { "afii10082",	0x0440 },
  { "afii10083",	0x0441 },
  { "afii10084",	0x0442 },
  { "afii10085",	0x0443 },
  { "afii10086",	0x0444 },
  { "afii10087",	0x0445 },
  { "afii10088",	0x0446 },
  { "afii10089",	0x0447 },
  { "afii10090",	0x0448 },
  { "afii10091",	0x0449 },
  { "afii10092",	0x044a },
  { "afii10093",	0x044b },
  { "afii10094",	0x044c },
  { "afii10095",	0x044d },
  { "afii10096",	0x044e },
  { "afii10097",	0x044f },
  { "afii10098",	0x0491 },
  { "afii10099",	0x0452 },
  { "afii10100",	0x0453 },
  { "afii10101",	0x0454 },
  { "afii10102",	0x0455 },
  { "afii10103",	0x0456 },
  { "afii10104",	0x0457 },
  { "afii10105",	0x0458 },
  { "afii10106",	0x0459 },
  { "afii10107",	0x045a },
  { "afii10108",	0x045b },
  { "afii10109",	0x045c },
  { "afii10110",	0x045e },
  { "afii10145",	0x040f },
  { "afii10146",	0x0462 },
  { "afii10147",	0x0472 },
  { "afii10148",	0x0474 },
  { "afii10192",	0xf6c6 },
  { "afii10193",	0x045f },
  { "afii10194",	0x0463 },
  { "afii10195",	0x0473 },
  { "afii10196",	0x0475 },
  { "afii10831",	0xf6c7 },
  { "afii10832",	0xf6c8 },
  { "afii10846",	0x04d9 },
  { "afii299",		0x200e },
  { "afii300",		0x200f },
  { "afii301",		0x200d },
  { "afii57381",	0x066a },
  { "afii57388",	0x060c },
  { "afii57392",	0x0660 },
  { "afii57393",	0x0661 },
  { "afii57394",	0x0662 },
  { "afii57395",	0x0663 },
  { "afii57396",	0x0664 },
  { "afii57397",	0x0665 },
  { "afii57398",	0x0666 },
  { "afii57399",	0x0667 },
  { "afii57400",	0x0668 },
  { "afii57401",	0x0669 },
  { "afii57403",	0x061b },
  { "afii57407",	0x061f },
  { "afii57409",	0x0621 },
  { "afii57410",	0x0622 },
  { "afii57411",	0x0623 },
  { "afii57412",	0x0624 },
  { "afii57413",	0x0625 },
  { "afii57414",	0x0626 },
  { "afii57415",	0x0627 },
  { "afii57416",	0x0628 },
  { "afii57417",	0x0629 },
  { "afii57418",	0x062a },
  { "afii57419",	0x062b },
  { "afii57420",	0x062c },
  { "afii57421",	0x062d },
  { "afii57422",	0x062e },
  { "afii57423",	0x062f },
  { "afii57424",	0x0630 },
  { "afii57425",	0x0631 },
  { "afii57426",	0x0632 },
  { "afii57427",	0x0633 },
  { "afii57428",	0x0634 },
  { "afii57429",	0x0635 },
  { "afii57430",	0x0636 },
  { "afii57431",	0x0637 },
  { "afii57432",	0x0638 },
  { "afii57433",	0x0639 },
  { "afii57434",	0x063a },
  { "afii57440",	0x0640 },
  { "afii57441",	0x0641 },
  { "afii57442",	0x0642 },
  { "afii57443",	0x0643 },
  { "afii57444",	0x0644 },
  { "afii57445",	0x0645 },
  { "afii57446",	0x0646 },
  { "afii57448",	0x0648 },
  { "afii57449",	0x0649 },
  { "afii57450",	0x064a },
  { "afii57451",	0x064b },
  { "afii57452",	0x064c },
  { "afii57453",	0x064d },
  { "afii57454",	0x064e },
  { "afii57455",	0x064f },
  { "afii57456",	0x0650 },
  { "afii57457",	0x0651 },
  { "afii57458",	0x0652 },
  { "afii57470",	0x0647 },
  { "afii57505",	0x06a4 },
  { "afii57506",	0x067e },
  { "afii57507",	0x0686 },
  { "afii57508",	0x0698 },
  { "afii57509",	0x06af },
  { "afii57511",	0x0679 },
  { "afii57512",	0x0688 },
  { "afii57513",	0x0691 },
  { "afii57514",	0x06ba },
  { "afii57519",	0x06d2 },
  { "afii57534",	0x06d5 },
  { "afii57636",	0x20aa },
  { "afii57645",	0x05be },
  { "afii57658",	0x05c3 },
  { "afii57664",	0x05d0 },
  { "afii57665",	0x05d1 },
  { "afii57666",	0x05d2 },
  { "afii57667",	0x05d3 },
  { "afii57668",	0x05d4 },
  { "afii57669",	0x05d5 },
  { "afii57670",	0x05d6 },
  { "afii57671",	0x05d7 },
  { "afii57672",	0x05d8 },
  { "afii57673",	0x05d9 },
  { "afii57674",	0x05da },
  { "afii57675",	0x05db },
  { "afii57676",	0x05dc },
  { "afii57677",	0x05dd },
  { "afii57678",	0x05de },
  { "afii57679",	0x05df },
  { "afii57680",	0x05e0 },
  { "afii57681",	0x05e1 },
  { "afii57682",	0x05e2 },
  { "afii57683",	0x05e3 },
  { "afii57684",	0x05e4 },
  { "afii57685",	0x05e5 },
  { "afii57686",	0x05e6 },
  { "afii57687",	0x05e7 },
  { "afii57688",	0x05e8 },
  { "afii57689",	0x05e9 },
  { "afii57690",	0x05ea },
  { "afii57694",	0xfb2a },
  { "afii57695",	0xfb2b },
  { "afii57700",	0xfb4b },
  { "afii57705",	0xfb1f },
  { "afii57716",	0x05f0 },
  { "afii57717",	0x05f1 },
  { "afii57718",	0x05f2 },
  { "afii57723",	0xfb35 },
  { "afii57793",	0x05b4 },
  { "afii57794",	0x05b5 },
  { "afii57795",	0x05b6 },
  { "afii57796",	0x05bb },
  { "afii57797",	0x05b8 },
  { "afii57798",	0x05b7 },
  { "afii57799",	0x05b0 },
  { "afii57800",	0x05b2 },
  { "afii57801",	0x05b1 },
  { "afii57802",	0x05b3 },
  { "afii57803",	0x05c2 },
  { "afii57804",	0x05c1 },
  { "afii57806",	0x05b9 },
  { "afii57807",	0x05bc },
  { "afii57839",	0x05bd },
  { "afii57841",	0x05bf },
  { "afii57842",	0x05c0 },
  { "afii57929",	0x02bc },
  { "afii61248",	0x2105 },
  { "afii61289",	0x2113 },
  { "afii61352",	0x2116 },
  { "afii61573",	0x202c },
  { "afii61574",	0x202d },
  { "afii61575",	0x202e },
  { "afii61664",	0x200c },
  { "afii63167",	0x066d },
  { "afii64937",	0x02bd },
  { "agrave",		0x00e0 },
  { "aleph",		0x2135 },
  { "alpha",		0x03b1 },
  { "alphatonos",	0x03ac },
  { "amacron",		0x0101 },
  { "ampersand",	0x0026 },
  { "ampersandsmall",	0xf726 },
  { "angle",		0x2220 },
  { "angleleft",	0x2329 },
  { "angleright",	0x232a },
  { "anoteleia",	0x0387 },
  { "aogonek",		0x0105 },
  { "approxequal",	0x2248 },
  { "aring",		0x00e5 },
  { "aringacute",	0x01fb },
  { "arrowboth",	0x2194 },
  { "arrowdblboth",	0x21d4 },
  { "arrowdbldown",	0x21d3 },
  { "arrowdblleft",	0x21d0 },
  { "arrowdblright",	0x21d2 },
  { "arrowdblup",	0x21d1 },
  { "arrowdown",	0x2193 },
  { "arrowhorizex",	0xf8e7 },
  { "arrowleft",	0x2190 },
  { "arrowright",	0x2192 },
  { "arrowup",		0x2191 },
  { "arrowupdn",	0x2195 },
  { "arrowupdnbse",	0x21a8 },
  { "arrowvertex",	0xf8e6 },
  { "asciicircum",	0x005e },
  { "asciitilde",	0x007e },
  { "asterisk",		0x002a },
  { "asteriskmath",	0x2217 },
  { "asuperior",	0xf6e9 },
  { "at",		0x0040 },
  { "atilde",		0x00e3 },
  { "b",		0x0062 },
  { "backslash",	0x005c },
  { "bar",		0x007c },
  { "beta",		0x03b2 },
  { "block",		0x2588 },
  { "braceex",		0xf8f4 },
  { "braceleft",	0x007b },
  { "braceleftbt",	0xf8f3 },
  { "braceleftmid",	0xf8f2 },
  { "bracelefttp",	0xf8f1 },
  { "braceright",	0x007d },
  { "bracerightbt",	0xf8fe },
  { "bracerightmid",	0xf8fd },
  { "bracerighttp",	0xf8fc },
  { "bracketleft",	0x005b },
  { "bracketleftbt",	0xf8f0 },
  { "bracketleftex",	0xf8ef },
  { "bracketlefttp",	0xf8ee },
  { "bracketright",	0x005d },
  { "bracketrightbt",	0xf8fb },
  { "bracketrightex",	0xf8fa },
  { "bracketrighttp",	0xf8f9 },
  { "breve",		0x02d8 },
  { "brokenbar",	0x00a6 },
  { "bsuperior",	0xf6ea },
  { "bullet",		0x2022 },
  { "c",		0x0063 },
  { "cacute",		0x0107 },
  { "caron",		0x02c7 },
  { "carriagereturn",	0x21b5 },
  { "ccaron",		0x010d },
  { "ccedilla",		0x00e7 },
  { "ccircumflex",	0x0109 },
  { "cdotaccent",	0x010b },
  { "cedilla",		0x00b8 },
  { "cent",		0x00a2 },
  { "centinferior",	0xf6df },
  { "centoldstyle",	0xf7a2 },
  { "centsuperior",	0xf6e0 },
  { "chi",		0x03c7 },
  { "circle",		0x25cb },
  { "circlemultiply",	0x2297 },
  { "circleplus",	0x2295 },
  { "circumflex",	0x02c6 },
  { "club",		0x2663 },
  { "colon",		0x003a },
  { "colonmonetary",	0x20a1 },
  { "comma",		0x002c },
  { "commaaccent",	0xf6c3 },
  { "commainferior",	0xf6e1 },
  { "commasuperior",	0xf6e2 },
  { "congruent",	0x2245 },
  { "copyright",	0x00a9 },
  { "copyrightsans",	0xf8e9 },
  { "copyrightserif",	0xf6d9 },
  { "currency",		0x00a4 },
  { "cyrBreve",		0xf6d1 },
  { "cyrFlex",		0xf6d2 },
  { "cyrbreve",		0xf6d4 },
  { "cyrflex",		0xf6d5 },
  { "d",		0x0064 },
  { "dagger",		0x2020 },
  { "daggerdbl",	0x2021 },
  { "dblGrave",		0xf6d3 },
  { "dblgrave",		0xf6d6 },
  { "dcaron",		0x010f },
  { "dcroat",		0x0111 },
  { "degree",		0x00b0 },
  { "delta",		0x03b4 },
  { "diamond",		0x2666 },
  { "dieresis",		0x00a8 },
  { "dieresisacute",	0xf6d7 },
  { "dieresisgrave",	0xf6d8 },
  { "dieresistonos",	0x0385 },
  { "divide",		0x00f7 },
  { "dkshade",		0x2593 },
  { "dnblock",		0x2584 },
  { "dollar",		0x0024 },
  { "dollarinferior",	0xf6e3 },
  { "dollaroldstyle",	0xf724 },
  { "dollarsuperior",	0xf6e4 },
  { "dong",		0x20ab },
  { "dotaccent",	0x02d9 },
  { "dotbelowcomb",	0x0323 },
  { "dotlessi",		0x0131 },
  { "dotlessj",		0xf6be },
  { "dotmath",		0x22c5 },
  { "dsuperior",	0xf6eb },
  { "e",		0x0065 },
  { "eacute",		0x00e9 },
  { "ebreve",		0x0115 },
  { "ecaron",		0x011b },
  { "ecircumflex",	0x00ea },
  { "edieresis",	0x00eb },
  { "edotaccent",	0x0117 },
  { "egrave",		0x00e8 },
  { "eight",		0x0038 },
  { "eightinferior",	0x2088 },
  { "eightoldstyle",	0xf738 },
  { "eightsuperior",	0x2078 },
  { "element",		0x2208 },
  { "ellipsis",		0x2026 },
  { "emacron",		0x0113 },
  { "emdash",		0x2014 },
  { "emptyset",		0x2205 },
  { "endash",		0x2013 },
  { "eng",		0x014b },
  { "eogonek",		0x0119 },
  { "epsilon",		0x03b5 },
  { "epsilontonos",	0x03ad },
  { "equal",		0x003d },
  { "equivalence",	0x2261 },
  { "estimated",	0x212e },
  { "esuperior",	0xf6ec },
  { "eta",		0x03b7 },
  { "etatonos",		0x03ae },
  { "eth",		0x00f0 },
  { "exclam",		0x0021 },
  { "exclamdbl",	0x203c },
  { "exclamdown",	0x00a1 },
  { "exclamdownsmall",	0xf7a1 },
  { "exclamsmall",	0xf721 },
  { "existential",	0x2203 },
  { "f",		0x0066 },
  { "female",		0x2640 },
  { "ff",		0xfb00 },
  { "ffi",		0xfb03 },
  { "ffl",		0xfb04 },
  { "fi",		0xfb01 },
  { "figuredash",	0x2012 },
  { "filledbox",	0x25a0 },
  { "filledrect",	0x25ac },
  { "five",		0x0035 },
  { "fiveeighths",	0x215d },
  { "fiveinferior",	0x2085 },
  { "fiveoldstyle",	0xf735 },
  { "fivesuperior",	0x2075 },
  { "fl",		0xfb02 },
  { "florin",		0x0192 },
  { "four",		0x0034 },
  { "fourinferior",	0x2084 },
  { "fouroldstyle",	0xf734 },
  { "foursuperior",	0x2074 },
  { "fraction",		0x2044 },
  { "fraction",		0x2215 },
  { "franc",		0x20a3 },
  { "g",		0x0067 },
  { "gamma",		0x03b3 },
  { "gbreve",		0x011f },
  { "gcaron",		0x01e7 },
  { "gcircumflex",	0x011d },
  { "gcommaaccent",	0x0123 },
  { "gdotaccent",	0x0121 },
  { "germandbls",	0x00df },
  { "gradient",		0x2207 },
  { "grave",		0x0060 },
  { "gravecomb",	0x0300 },
  { "greater",		0x003e },
  { "greaterequal",	0x2265 },
  { "guillemotleft",	0x00ab },
  { "guillemotright",	0x00bb },
  { "guilsinglleft",	0x2039 },
  { "guilsinglright",	0x203a },
  { "h",		0x0068 },
  { "hbar",		0x0127 },
  { "hcircumflex",	0x0125 },
  { "heart",		0x2665 },
  { "hookabovecomb",	0x0309 },
  { "house",		0x2302 },
  { "hungarumlaut",	0x02dd },
  { "hyphen",		0x002d },
  { "hypheninferior",	0xf6e5 },
  { "hyphensuperior",	0xf6e6 },
  { "i",		0x0069 },
  { "iacute",		0x00ed },
  { "ibreve",		0x012d },
  { "icircumflex",	0x00ee },
  { "idieresis",	0x00ef },
  { "igrave",		0x00ec },
  { "ij",		0x0133 },
  { "imacron",		0x012b },
  { "infinity",		0x221e },
  { "integral",		0x222b },
  { "integralbt",	0x2321 },
  { "integralex",	0xf8f5 },
  { "integraltp",	0x2320 },
  { "intersection",	0x2229 },
  { "invbullet",	0x25d8 },
  { "invcircle",	0x25d9 },
  { "invsmileface",	0x263b },
  { "iogonek",		0x012f },
  { "iota",		0x03b9 },
  { "iotadieresis",	0x03ca },
  { "iotadieresistonos", 0x0390 },
  { "iotatonos",	0x03af },
  { "isuperior",	0xf6ed },
  { "itilde",		0x0129 },
  { "j",		0x006a },
  { "jcircumflex",	0x0135 },
  { "k",		0x006b },
  { "kappa",		0x03ba },
  { "kcommaaccent",	0x0137 },
  { "kgreenlandic",	0x0138 },
  { "l",		0x006c },
  { "lacute",		0x013a },
  { "lambda",		0x03bb },
  { "lcaron",		0x013e },
  { "lcommaaccent",	0x013c },
  { "ldot",		0x0140 },
  { "less",		0x003c },
  { "lessequal",	0x2264 },
  { "lfblock",		0x258c },
  { "lira",		0x20a4 },
  { "ll",		0xf6c0 },
  { "logicaland",	0x2227 },
  { "logicalnot",	0x00ac },
  { "logicalor",	0x2228 },
  { "longs",		0x017f },
  { "lozenge",		0x25ca },
  { "lslash",		0x0142 },
  { "lsuperior",	0xf6ee },
  { "ltshade",		0x2591 },
  { "m",		0x006d },
  { "macron",		0x00af },
  { "macron",		0x02c9 },
  { "male",		0x2642 },
  { "minus",		0x00ad },
  { "minus",		0x2212 },
  { "minute",		0x2032 },
  { "msuperior",	0xf6ef },
  { "mu",		0x00b5 },
  { "mu",		0x03bc },
  { "multiply",		0x00d7 },
  { "musicalnote",	0x266a },
  { "musicalnotedbl",	0x266b },
  { "n",		0x006e },
  { "nacute",		0x0144 },
  { "napostrophe",	0x0149 },
  { "ncaron",		0x0148 },
  { "ncommaaccent",	0x0146 },
  { "nine",		0x0039 },
  { "nineinferior",	0x2089 },
  { "nineoldstyle",	0xf739 },
  { "ninesuperior",	0x2079 },
  { "notelement",	0x2209 },
  { "notequal",		0x2260 },
  { "notsubset",	0x2284 },
  { "nsuperior",	0x207f },
  { "ntilde",		0x00f1 },
  { "nu",		0x03bd },
  { "numbersign",	0x0023 },
  { "o",		0x006f },
  { "oacute",		0x00f3 },
  { "obreve",		0x014f },
  { "ocircumflex",	0x00f4 },
  { "odieresis",	0x00f6 },
  { "oe",		0x0153 },
  { "ogonek",		0x02db },
  { "ograve",		0x00f2 },
  { "ohorn",		0x01a1 },
  { "ohungarumlaut",	0x0151 },
  { "omacron",		0x014d },
  { "omega",		0x03c9 },
  { "omega1",		0x03d6 },
  { "omegatonos",	0x03ce },
  { "omicron",		0x03bf },
  { "omicrontonos",	0x03cc },
  { "one",		0x0031 },
  { "onedotenleader",	0x2024 },
  { "oneeighth",	0x215b },
  { "onefitted",	0xf6dc },
  { "onehalf",		0x00bd },
  { "oneinferior",	0x2081 },
  { "oneoldstyle",	0xf731 },
  { "onequarter",	0x00bc },
  { "onesuperior",	0x00b9 },
  { "onethird",		0x2153 },
  { "openbullet",	0x25e6 },
  { "ordfeminine",	0x00aa },
  { "ordmasculine",	0x00ba },
  { "orthogonal",	0x221f },
  { "oslash",		0x00f8 },
  { "oslashacute",	0x01ff },
  { "osuperior",	0xf6f0 },
  { "otilde",		0x00f5 },
  { "p",		0x0070 },
  { "paragraph",	0x00b6 },
  { "parenleft",	0x0028 },
  { "parenleftbt",	0xf8ed },
  { "parenleftex",	0xf8ec },
  { "parenleftinferior", 0x208d },
  { "parenleftsuperior", 0x207d },
  { "parenlefttp",	0xf8eb },
  { "parenright",	0x0029 },
  { "parenrightbt",	0xf8f8 },
  { "parenrightex",	0xf8f7 },
  { "parenrightinferior", 0x208e },
  { "parenrightsuperior", 0x207e },
  { "parenrighttp",	0xf8f6 },
  { "partialdiff",	0x2202 },
  { "percent",		0x0025 },
  { "period",		0x002e },
  { "periodcentered",	0x00b7 },
  { "periodcentered",	0x2219 },
  { "periodinferior",	0xf6e7 },
  { "periodsuperior",	0xf6e8 },
  { "perpendicular",	0x22a5 },
  { "perthousand",	0x2030 },
  { "peseta",		0x20a7 },
  { "phi",		0x03c6 },
  { "phi1",		0x03d5 },
  { "pi",		0x03c0 },
  { "plus",		0x002b },
  { "plusminus",	0x00b1 },
  { "prescription",	0x211e },
  { "product",		0x220f },
  { "propersubset",	0x2282 },
  { "propersuperset",	0x2283 },
  { "proportional",	0x221d },
  { "psi",		0x03c8 },
  { "q",		0x0071 },
  { "question",		0x003f },
  { "questiondown",	0x00bf },
  { "questiondownsmall", 0xf7bf },
  { "questionsmall",	0xf73f },
  { "quotedbl",		0x0022 },
  { "quotedblbase",	0x201e },
  { "quotedblleft",	0x201c },
  { "quotedblright",	0x201d },
  { "quoteleft",	0x2018 },
  { "quotereversed",	0x201b },
  { "quoteright",	0x2019 },
  { "quotesinglbase",	0x201a },
  { "quotesingle",	0x0027 },
  { "r",		0x0072 },
  { "racute",		0x0155 },
  { "radical",		0x221a },
  { "radicalex",	0xf8e5 },
  { "rcaron",		0x0159 },
  { "rcommaaccent",	0x0157 },
  { "reflexsubset",	0x2286 },
  { "reflexsuperset",	0x2287 },
  { "registered",	0x00ae },
  { "registersans",	0xf8e8 },
  { "registerserif",	0xf6da },
  { "revlogicalnot",	0x2310 },
  { "rho",		0x03c1 },
  { "ring",		0x02da },
  { "rsuperior",	0xf6f1 },
  { "rtblock",		0x2590 },
  { "rupiah",		0xf6dd },
  { "s",		0x0073 },
  { "sacute",		0x015b },
  { "scaron",		0x0161 },
  { "scedilla",		0x015f },
  { "scedilla",		0xf6c2 },
  { "scircumflex",	0x015d },
  { "scommaaccent",	0x0219 },
  { "second",		0x2033 },
  { "section",		0x00a7 },
  { "semicolon",	0x003b },
  { "seven",		0x0037 },
  { "seveneighths",	0x215e },
  { "seveninferior",	0x2087 },
  { "sevenoldstyle",	0xf737 },
  { "sevensuperior",	0x2077 },
  { "shade",		0x2592 },
  { "sigma",		0x03c3 },
  { "sigma1",		0x03c2 },
  { "similar",		0x223c },
  { "six",		0x0036 },
  { "sixinferior",	0x2086 },
  { "sixoldstyle",	0xf736 },
  { "sixsuperior",	0x2076 },
  { "slash",		0x002f },
  { "smileface",	0x263a },
  { "space",		0x0020 },
  { "space",		0x00a0 },
  { "spade",		0x2660 },
  { "ssuperior",	0xf6f2 },
  { "sterling",		0x00a3 },
  { "suchthat",		0x220b },
  { "summation",	0x2211 },
  { "sun",		0x263c },
  { "t",		0x0074 },
  { "tau",		0x03c4 },
  { "tbar",		0x0167 },
  { "tcaron",		0x0165 },
  { "tcommaaccent",	0x0163 },
  { "tcommaaccent",	0x021b },
  { "therefore",	0x2234 },
  { "theta",		0x03b8 },
  { "theta1",		0x03d1 },
  { "thorn",		0x00fe },
  { "three",		0x0033 },
  { "threeeighths",	0x215c },
  { "threeinferior",	0x2083 },
  { "threeoldstyle",	0xf733 },
  { "threequarters",	0x00be },
  { "threequartersemdash", 0xf6de },
  { "threesuperior",	0x00b3 },
  { "tilde",		0x02dc },
  { "tildecomb",	0x0303 },
  { "tonos",		0x0384 },
  { "trademark",	0x2122 },
  { "trademarksans",	0xf8ea },
  { "trademarkserif",	0xf6db },
  { "triagdn",		0x25bc },
  { "triaglf",		0x25c4 },
  { "triagrt",		0x25ba },
  { "triagup",		0x25b2 },
  { "tsuperior",	0xf6f3 },
  { "two",		0x0032 },
  { "twodotenleader",	0x2025 },
  { "twoinferior",	0x2082 },
  { "twooldstyle",	0xf732 },
  { "twosuperior",	0x00b2 },
  { "twothirds",	0x2154 },
  { "u",		0x0075 },
  { "uacute",		0x00fa },
  { "ubreve",		0x016d },
  { "ucircumflex",	0x00fb },
  { "udieresis",	0x00fc },
  { "ugrave",		0x00f9 },
  { "uhorn",		0x01b0 },
  { "uhungarumlaut",	0x0171 },
  { "umacron",		0x016b },
  { "underscore",	0x005f },
  { "underscoredbl",	0x2017 },
  { "union",		0x222a },
  { "universal",	0x2200 },
  { "uogonek",		0x0173 },
  { "upblock",		0x2580 },
  { "upsilon",		0x03c5 },
  { "upsilondieresis",	0x03cb },
  { "upsilondieresistonos", 0x03b0 },
  { "upsilontonos",	0x03cd },
  { "uring",		0x016f },
  { "utilde",		0x0169 },
  { "v",		0x0076 },
  { "w",		0x0077 },
  { "wacute",		0x1e83 },
  { "wcircumflex",	0x0175 },
  { "wdieresis",	0x1e85 },
  { "weierstrass",	0x2118 },
  { "wgrave",		0x1e81 },
  { "x",		0x0078 },
  { "xi",		0x03be },
  { "y",		0x0079 },
  { "yacute",		0x00fd },
  { "ycircumflex",	0x0177 },
  { "ydieresis",	0x00ff },
  { "yen",		0x00a5 },
  { "ygrave",		0x1ef3 },
  { "z",		0x007a },
  { "zacute",		0x017a },
  { "zcaron",		0x017e },
  { "zdotaccent",	0x017c },
  { "zero",		0x0030 },
  { "zeroinferior",	0x2080 },
  { "zerooldstyle",	0xf730 },
  { "zerosuperior",	0x2070 },
  { "zeta",		0x03b6 }
};

// --- Text State ---

//
// 'device_begin_text()' - Handles the BT (Begin Text) operator, 
// 			   initializing a new text object
//

void 						  // O - Void
device_begin_text(p2c_device_t *dev)		// I - Active Rendering Context
{
  if (g_verbose)
    printf("DEBUG: Begin Text Object\n");

  // Access the current graphics state from the top of the device stack.
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];

  // Reset the text_matrix (current glyph position) to identity matrices.
  cairo_matrix_init_identity(&gs->text_matrix);

  // Resets the text_line_matrix (start of the current line) to identity matrices.
  cairo_matrix_init_identity(&gs->text_line_matrix);
}

//
// 'device_end_text()' - Handles the ET (End Text) operator.
//

void 						  // O - Void
device_end_text(p2c_device_t *dev)		// I - Active Rendering Context
{
  // TODO: Currently serves as a placeholder for cleanup or finalizing text object diagnostics
  if (g_verbose)
    printf("DEBUG: End Text Object\n");
}

//
//  'device_set_text_leading()' - Sets the vertical spacing used for newline operations
//

void 							  // O - Void
device_set_text_leading(p2c_device_t *dev, 		// I - Active Rendering Context
			double leading)			// I - Spacing length 
{
  if (g_verbose)
    printf("DEBUG: Set Text Leading to %f\n", leading);

  // Update the text_leading value in the active graphics state for current stack level.
  dev->gstack[dev->gstack_ptr].text_leading = leading;
}

//
// 'device_move_text_cursor()' - Offsets the text position (Td operator).
//

void 							  // O - Void
device_move_text_cursor(p2c_device_t *dev, 		// I - Active Rendering Context
			double tx, double ty)		// I - coordinates to position
{
  if (g_verbose)
    printf("DEBUG: Move Text Cursor by (%f, %f)\n", tx, ty);

  // Initialize a temporary translation matrix using the provided tx and ty coordinates.
  cairo_matrix_t trans_matrix;
  cairo_matrix_init_translate(&trans_matrix, tx, ty);

  // Access the current graphics state from the top of the device stack.
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  
  // Multiply the existing text_line_matrix by translation to find the new line start.
  cairo_matrix_multiply(&gs->text_line_matrix, &trans_matrix, &gs->text_line_matrix);

  // Synchronize text_matrix with the updated line matrix to move the drawing cursor
  memcpy(&gs->text_matrix, &gs->text_line_matrix, sizeof(cairo_matrix_t));
}

//
// 'device_next_line()' - Move the cursor to the start of the next line (T* operator).
//

void 							  // O - Void
device_next_line(p2c_device_t *dev)			// I - Active Rendering Context
{
  if (g_verbose)
    printf("DEBUG: Move to Next Line\n");

  // Access the current graphics state from the top of the device stack.
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];

  // Call with a vertical offset of -leading (moving down the page).
  device_move_text_cursor(dev, 0, -gs->text_leading);
}

//
// 'device_set_text_matrix()' - Explicitly set text transformation matrix (Tm operator).
//

void 							  // O - Void
device_set_text_matrix(p2c_device_t *dev, 		// I - Active Rendering Context
		       double a, double b, 		// I - Coefficients for matrix
		       double c, double d,
		       double e, double f)
{
  if (g_verbose)
    printf("DEBUG: Set Text Matrix to [%f %f %f %f %f %f]\n", a, b, c, d, e, f);

  // Access the current graphics state from the top of the device stack.
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  
  // Initialize matrix using the six provided coefficients
  cairo_matrix_init(&gs->text_matrix, a, b, c, d, e, f);

  // Copies this matrix to set a new baseline reference
  memcpy(&gs->text_line_matrix, &gs->text_matrix, sizeof(cairo_matrix_t));
}

// 
// 'load_default_font()' - Internal fallback to load system fonts if PDF 
// 			   embedding is missing.
//

static bool 					  // O - 0 if failed, 1 if successful	
load_default_font(p2c_device_t *dev, 		// I - Active Rendering Context
		  double font_size)		// I - size of Font
{
  // Access the current graphics state from the top of the device stack.
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
    
  const char *default_fonts[] = {
    "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
    "/usr/share/fonts/TTF/DejaVuSans.ttf",
    "/System/Library/Fonts/Helvetica.ttc",
    "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
    "/usr/share/fonts/truetype/freefont/FreeSans.ttf",
    NULL
  };
    
  // Iterates through a list of common system font paths (e.g., DejaVuSans, Helvetica).
  for (int i = 0; default_fonts[i] != NULL; i++)
  {
    // Load the font into FreeType.
    FT_Error ft_error = FT_New_Face(dev->ft_library, default_fonts[i], 0, &gs->ft_face);
      if (ft_error == 0)
      {
        fprintf(stderr, "DEBUG: ✓ Loaded default font: %s\n", default_fonts[i]);
            
	// Sets the character size to match the PDF request at 72 DPI.
        FT_Set_Char_Size(gs->ft_face, 0, (FT_F26Dot6)(font_size * 64), 72, 72);
            
        // Set encoding map
        gs->encoding_map = NULL;
            
        return true;
      }
  }
    
  fprintf(stderr, "DEBUG: ✗ Could not load any default font\n");
  return false;
}

//
// 'load_embedded_font()' - Extracts and loads font data directly from the PDF file.
//

static bool 					 	  // O - 0 if failed, 1 if success
load_embedded_font(p2c_device_t *dev, 			// I - Active Rendering Context	
		   pdfio_obj_t *font_obj, 		// I - Font Object for Page
		   double font_size)			// I - Size of Font
{
  // Access the current graphics state from the top of the device stack.
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  pdfio_dict_t *font_dict = pdfioObjGetDict(font_obj);
    
  if (!font_dict)
  {
    fprintf(stderr, "DEBUG: ✗ Font object has no dictionary\n");
    return false;
  }
    
  // Get FontDescriptor
  pdfio_obj_t *font_descriptor = pdfioDictGetObj(font_dict, "FontDescriptor");
  if (!font_descriptor)
  { 
    fprintf(stderr, "DEBUG: No FontDescriptor found\n");
    return false;
  }
    
  pdfio_dict_t *descriptor_dict = pdfioObjGetDict(font_descriptor);
  if (!descriptor_dict)
  {
    fprintf(stderr, "DEBUG: ✗ FontDescriptor has no dictionary\n");
    return false;
  }
    
  // Check for embedded font stream (FontFile2 for TrueType, FontFile for Type1)
  pdfio_obj_t *font_file = pdfioDictGetObj(descriptor_dict, "FontFile2");
  if (!font_file)
  {
    font_file = pdfioDictGetObj(descriptor_dict, "FontFile");
  }
  if (!font_file)
  {
    font_file = pdfioDictGetObj(descriptor_dict, "FontFile3");
  }
    
  if (!font_file)
  {
    fprintf(stderr, "DEBUG: No embedded font stream found\n");
    return false;
  }
    
  fprintf(stderr, "DEBUG: ✓ Found embedded font (obj %zu)\n", pdfioObjGetNumber(font_file));
    
  // Get the stream dictionary to find the length
  pdfio_dict_t *stream_dict = pdfioObjGetDict(font_file);
  if (!stream_dict)
  {
    fprintf(stderr, "DEBUG: ✗ Font stream has no dictionary\n");
    return false;
  }
    
  size_t font_data_length = (size_t)pdfioDictGetNumber(stream_dict, "Length");
  if (font_data_length == 0)
  {
    fprintf(stderr, "DEBUG: ✗ Font stream length is 0\n");
    return false;
  }
    
  fprintf(stderr, "DEBUG: Font data length: %zu bytes\n", font_data_length);

  // Allocate memory for font data with extra buffer
  unsigned char *font_data = malloc(font_data_length + 1024);
  if (!font_data)
  {
    fprintf(stderr, "DEBUG: ✗ Failed to allocate memory for font data\n");
    return false;
  }

  // Open and read the font stream
  pdfio_stream_t *font_stream = pdfioObjOpenStream(font_file, true);
  if (!font_stream)
  {
    fprintf(stderr, "DEBUG: ✗ Failed to open font stream\n");
    free(font_data);
    return false;
  }

  // Read all font data into memory
  ssize_t bytes_read = pdfioStreamRead(font_stream, font_data, font_data_length);

  // CRITICAL: Close the stream IMMEDIATELY to free the object
  pdfioStreamClose(font_stream);
  fprintf(stderr, "DEBUG: ✓ Font stream closed successfully\n");

  if (bytes_read <= 0)
  {
    fprintf(stderr, "DEBUG: ✗ Failed to read font data (read %zd bytes)\n", bytes_read);
    free(font_data);
    return false;
  }

  fprintf(stderr, "DEBUG: ✓ Read %zd bytes of font data\n", bytes_read);
  
  // Load font into FreeType
  FT_Error ft_error = FT_New_Memory_Face(
      dev->ft_library,
      font_data,
      bytes_read,
      0,  // face_index
      &gs->ft_face
  );
    
  if (ft_error != 0)
  {
    fprintf(stderr, "DEBUG: ✗ FreeType error loading font: %d\n", ft_error);
    free(font_data);
    return false;
  }
    
  fprintf(stderr, "DEBUG: ✓ Font loaded into FreeType successfully!\n");
    
  // Set the font size
  FT_Set_Char_Size(gs->ft_face, 0, (FT_F26Dot6)(font_size * 64), 72, 72);
   
  // Set encoding map
//  gs->encoding_map = WIN_ANSI_ENCODING;
  
  // Store font data pointer so we can free it later
  // You'll need to add a field to graphics_state_t: unsigned char *font_data;
  gs->font_data = font_data;
   
  fprintf(stderr, "DEBUG: Font size set to %.2f\n", font_size);
  return true;
}

//
// 'find_font_object()' - Searches for a font definition in the PDF structure.
//

static pdfio_obj_t* 				  // O - Return Font Object
find_font_object(p2c_device_t *dev, 		// I - Active Rendering Context
		 const char *font_name)		// I - Name of Font
{
  pdfio_obj_t *font_obj = NULL;
    
  // Primary lookup: Check dev->font_dict first
  if (dev->font_dict)
  {
    fprintf(stderr, "DEBUG: Trying primary lookup in dev->font_dict...\n");
    font_obj = pdfioDictGetObj(dev->font_dict, font_name);
    if (font_obj)
    {
      fprintf(stderr, "DEBUG: ✓ Font '%s' found in dev->font_dict!\n", font_name);
      return font_obj;
    }
    fprintf(stderr, "DEBUG: ✗ Font '%s' NOT found in dev->font_dict\n", font_name);
  }
  else
  {
    fprintf(stderr, "DEBUG: dev->font_dict is NULL, skipping primary lookup\n");
  }
    
  // Fallback: Walk up the page tree
  fprintf(stderr, "DEBUG: Starting fallback - walking page tree...\n");
  pdfio_obj_t *current_page = dev->page;
  int level = 0;
    
  while (current_page && !font_obj && level < 10)
  {
    fprintf(stderr, "DEBUG: [Level %d] Checking page object %zu...\n", 
		    level, pdfioObjGetNumber(current_page));
   
    pdfio_dict_t *page_dict = pdfioObjGetDict(current_page);
    if (!page_dict)
    {
      fprintf(stderr, "DEBUG: [Level %d] Page dict is NULL\n", level);
      break;
    }
   
    // Get Resources dictionary
    pdfio_dict_t *resources_dict = NULL;
    pdfio_obj_t *resources_obj = pdfioDictGetObj(page_dict, "Resources");
        
    if (resources_obj)
    {
      fprintf(stderr, "DEBUG: [Level %d] Resources found as indirect object %zu\n", 
                      level, pdfioObjGetNumber(resources_obj));
      resources_dict = pdfioObjGetDict(resources_obj);
    }
    else
    {
      resources_dict = pdfioDictGetDict(page_dict, "Resources");
      if (resources_dict)
      {
        fprintf(stderr, "DEBUG: [Level %d] Resources found as direct dictionary\n", level);
      }
    }
   
    if (resources_dict)
    {
      // Get Font dictionary
      pdfio_dict_t *font_dict = NULL;
      pdfio_obj_t *font_dict_obj = pdfioDictGetObj(resources_dict, "Font");

      if (font_dict_obj)
      {
        fprintf(stderr, "DEBUG: [Level %d] Font dict found as indirect object %zu\n", 
                        level, pdfioObjGetNumber(font_dict_obj));
       	font_dict = pdfioObjGetDict(font_dict_obj);
      }
      else
      {
        font_dict = pdfioDictGetDict(resources_dict, "Font");
       	if (font_dict)
       	{
          fprintf(stderr, "DEBUG: [Level %d] Font dict found as direct dictionary\n", level);
       	}
      }
     
      if (font_dict)
      {
        font_obj = pdfioDictGetObj(font_dict, font_name);
       	if (font_obj)
       	{
          fprintf(stderr, "DEBUG: [Level %d] ✓ Font '%s' found (obj %zu)!\n", 
                          level, font_name, pdfioObjGetNumber(font_obj));
	 
	  const char *subtype = pdfioObjGetSubtype(font_obj);
	  const char *basefont = pdfioDictGetName(pdfioObjGetDict(font_obj), "BaseFont");
	  fprintf(stderr, "DEBUG: Font Subtype: %s\n", subtype ? subtype : "(null)");
	  fprintf(stderr, "DEBUG: Font BaseFont: %s\n", basefont ? basefont : "(null)");
	 
	  return font_obj;
       	}
      }
    }
   
    // Move to parent
    current_page = pdfioDictGetObj(page_dict, "Parent");
    level++;
  }
 
  return NULL;
}

//
// 'device_set_font()' - Main interface to change the active font (Tf operator).
//

void 							  // O - Void
device_set_font(p2c_device_t *dev, 			// I - Active Rendering Context	
		const char *font_name, 			// I - Name of Font
		double font_size)			// I - Size of Font
{
    graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
    
    fprintf(stderr, "\n=== DEBUG: device_set_font START ===\n");
    fprintf(stderr, "DEBUG: Looking for font: '%s' with size: %.2f\n", font_name, font_size);
    
    // Clean up previous font if exists
    if (gs->ft_face)
    {
        FT_Done_Face(gs->ft_face);
        gs->ft_face = NULL;
    }
    if (gs->font_data)
    {
        free(gs->font_data);
        gs->font_data = NULL;
    }
    
    // Set font size
    gs->font_size = font_size;
    
    // Find the font object
    pdfio_obj_t *font_obj = find_font_object(dev, font_name);
    
    bool font_loaded = false;
    
    // Try to load embedded font
    if (font_obj)
    {
        font_loaded = load_embedded_font(dev, font_obj, font_size);
    }
    
    // If embedded font failed, try default system font
    if (!font_loaded)
    {
        fprintf(stderr, "DEBUG: Embedded font not available, trying default system font...\n");
        font_loaded = load_default_font(dev, font_size);
    }
    
    if (!font_loaded)
    {
        fprintf(stderr, "DEBUG: ✗ Failed to load any font - text will not render\n");
    }
    
    fprintf(stderr, "=== DEBUG: device_set_font END ===\n\n");
}

//
// '_device_show_text_internal' - The core engine that draws strings glyph by glyph.
//

static double						  // O - Advance width
_device_show_text_internal(p2c_device_t *dev, 		// I - Active Rendering Context
			   const char *str)		// I - Text String
{
  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];
  FT_Face ft_face = gs->ft_face;
  const char **encoding_map = gs->encoding_map;
  size_t len = strlen(str);
  
  fprintf(stderr, "DEBUG: _device_show_text_internal called with text: \"%s\"\n", str);
  fprintf(stderr, "DEBUG: ft_face = %p\n", (void*)ft_face);
  
  if (!ft_face)
  {
    fprintf(stderr, "DEBUG: Skipping text draw - ft_face is NULL (font not loaded)\n");
    return 0.0;
  }

  cairo_glyph_t *glyphs = malloc(len * sizeof(cairo_glyph_t));
  if (!glyphs) return 0.0;

  int num_glyphs = 0;
  double total_advance_x = 0.0;

  for (size_t i = 0; i < len; i++)
  {
    unsigned char char_code = (unsigned char)str[i];
    unsigned int glyph_index = 0;
    
    // Try direct Unicode mapping first (works for most TrueType fonts)
    glyph_index = FT_Get_Char_Index(ft_face, char_code);
    
    // If that fails and we have an encoding map, try glyph name lookup
    if (glyph_index == 0 && encoding_map && encoding_map[char_code])
    {
      const char *glyph_name = encoding_map[char_code];
      glyph_index = FT_Get_Name_Index(ft_face, (char *)glyph_name);
      
      if (g_verbose && glyph_index == 0)
        fprintf(stderr, "DEBUG: Glyph not found for char_code=%d, glyph_name=%s\n", 
                char_code, glyph_name);
    }
    
    // Fallback: use char_code directly as glyph index
    if (glyph_index == 0)
    {
      glyph_index = char_code;
    }

    glyphs[num_glyphs].index = glyph_index;
    glyphs[num_glyphs].x = total_advance_x;
    glyphs[num_glyphs].y = 0.0;  // Y position is 0 in glyph space
    num_glyphs++;

    if (FT_Load_Glyph(ft_face, glyph_index, FT_LOAD_DEFAULT) == 0)
    {
      total_advance_x += (double)ft_face->glyph->advance.x / 64.0;
    }
  }

  cairo_save(dev->cr);
  
  // Apply the text matrix transformation
  // The text matrix contains the position and any rotation/scaling
  cairo_matrix_t text_transform;
  cairo_matrix_init(&text_transform,
                    gs->text_matrix.xx,
                    gs->text_matrix.yx,
                    gs->text_matrix.xy,
                    gs->text_matrix.yy,
                    gs->text_matrix.x0,
                    gs->text_matrix.y0);
  cairo_transform(dev->cr, &text_transform);
  
  // Create a Cairo font face from FreeType face
  cairo_font_face_t *cairo_face = cairo_ft_font_face_create_for_ft_face(ft_face, 0);
  cairo_set_font_face(dev->cr, cairo_face);
  cairo_set_font_size(dev->cr, gs->font_size);
  
  // Set the fill color
  cairo_set_source_rgb(dev->cr, gs->fill_rgb[0], gs->fill_rgb[1], gs->fill_rgb[2]);
  
  // Render glyphs based on text rendering mode
  switch (gs->text_rendering_mode)
  {
    case 0:  // Fill
      cairo_show_glyphs(dev->cr, glyphs, num_glyphs);
      break;
    case 1:  // Stroke
      cairo_set_source_rgb(dev->cr, gs->stroke_rgb[0], gs->stroke_rgb[1], gs->stroke_rgb[2]);
      cairo_glyph_path(dev->cr, glyphs, num_glyphs);
      cairo_stroke(dev->cr);
      break;
    case 2:  // Fill then stroke
      cairo_glyph_path(dev->cr, glyphs, num_glyphs);
      cairo_set_source_rgb(dev->cr, gs->fill_rgb[0], gs->fill_rgb[1], gs->fill_rgb[2]);
      cairo_fill_preserve(dev->cr);
      cairo_set_source_rgb(dev->cr, gs->stroke_rgb[0], gs->stroke_rgb[1], gs->stroke_rgb[2]);
      cairo_stroke(dev->cr);
      break;
    case 3:  // Invisible
      // Do nothing
      break;
    default:  // Default to fill
      cairo_show_glyphs(dev->cr, glyphs, num_glyphs);
      break;
  }
  
  cairo_font_face_destroy(cairo_face);
  cairo_restore(dev->cr);
  free(glyphs);

  // Return the advance width in text space units
  return total_advance_x * (gs->font_size / (double)ft_face->units_per_EM);
}

//
// 'device_show_text()' - Renders text and updates cursor (Tj operator).
//

void 						  // O - Void
device_show_text(p2c_device_t *dev, 		// I - Active Rendering Context
		 const char *str)		// I - Text String
{
  if (g_verbose)
    printf("DEBUG: Show Text (Tj): \"%s\"\n", str);

  double advance = _device_show_text_internal(dev, str);
  cairo_matrix_translate(&dev->gstack[dev->gstack_ptr].text_matrix, advance, 0);
}

//
// 'device_show_text_kerning()' - Renders text with precise spacing adjustments (TJ operator).
//

void 							  // O - Void
device_show_text_kerning(p2c_device_t *dev, 		// I - Active Rendering Context
			 operand_t *operands, 		// I - Type of Operand(string, Numbers)	
			 int num_operands)		// I - Number of Operands
{
  if (g_verbose)
    printf("DEBUG: Show Text with Kerning (TJ)\n");

  graphics_state_t *gs = &dev->gstack[dev->gstack_ptr];

  for (int i = 0; i < num_operands; i++)
  {
    if (operands[i].type == OP_TYPE_STRING)
    {
      double advance = _device_show_text_internal(dev, operands[i].value.string);
      cairo_matrix_translate(&gs->text_matrix, advance, 0);
    }
    else if (operands[i].type == OP_TYPE_NUMBER)
    {
      double adjustment = -operands[i].value.number / 1000.0 * gs->font_size;
      if (g_verbose)
        printf("DEBUG: TJ applying kerning adjustment: %f units\n", adjustment);
      cairo_matrix_translate(&gs->text_matrix, adjustment, 0);
    }
  }
}

//
// 'device_set_text_rendering_mode()' - Sets whether text is filled, outlined, or 
// 					invisible (Tr operator).
//

void 							  // O - Void
device_set_text_rendering_mode(p2c_device_t *dev, 	// I - Active Rendering Context
			       int mode)		// I - Mode of Text visual
{
  if (g_verbose)
    printf("DEBUG: Set Text Rendering Mode to %d\n", mode);

  if (mode >= 0 && mode <= 7)
  {
    dev->gstack[dev->gstack_ptr].text_rendering_mode = mode;
  }
}
