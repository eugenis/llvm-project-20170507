//===----------------------------------------------------------------------===//
//
// ΚΚΚΚΚΚΚΚΚΚΚΚΚΚΚΚΚΚΚΚThe LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// <random>

// template<class Engine, size_t k>
// class shuffle_order_engine

// explicit shuffle_order_engine(result_type s = default_seed);

#include <random>
#include <sstream>
#include <cassert>

void
test1()
{
    const char* a = "1771550148 168070 677268843 1194115201 1259501992 "
        "703671065 407145426 1010275440 1693606898 1702877348 745024267 "
        "1793193459 416963415 664975744 742430420 1148079870 637806795 "
        "1527921388 165317290 1791337459 1435426120 375508442 1863429808 "
        "1910758855 653618747 991426424 578291095 1974930990 1157900898 "
        "343583572 25567821 221638147 1335692731 1341167826 1019292670 "
        "774852571 606325389 700907908 1211405961 1955012967 1403137269 "
        "1010152376 1772753897 486628401 1145807831 1106352968 1560917450 "
        "679350398 1819071734 1561434646 781928982 1427964481 1669276942 "
        "811199786 1608612146 1272705739 1428231253 1857946652 2097152784 "
        "197742477 1300609030 99924397 97128425 349867255 408729299 1860625187 "
        "2018133942 1420442476 1948474080 1025729457 1583749330 15184745 "
        "1806938869 1655319056 296727307 638820415 1383963552 880037807 "
        "1075545360 1321008721 1507631161 597371974 544717293 340756290 "
        "1899563128 1465595994 634440068 777915521 545718511 2135841687 "
        "1902073804 712854586 135760289 1095544109 285050585 1956649285 "
        "987446484 259432572 891434194 1488577086 330596852 801096775 "
        "1458514382 1872871416 1682074633 1153627723 1538775345 51662594 "
        "709823970 739804705 2114844452 1188863267 1037076781 1172179215 "
        "1948572574 533634468 902793804 1283497773 273836696 315894151 "
        "653420473 1954002600 1601768276 64415940 306945492 577163950 "
        "210874151 813838307 857078006 1737226413 376658679 1868110244 "
        "1117951768 1080937173 1746896638 1842856729 1883887269 2141922362 "
        "1020763473 1872318475 978729834 1935067665 1189895487 1205729145 "
        "1034046923 1788963337 188263312 898072753 1393688555 1119406056 "
        "1900835472 1375045132 1312008157 559007303 2142269543 413383599 "
        "628550348 573639243 1100665718 464587168 65992084 1027393936 "
        "1641360472 1918007189 69800406 609352380 35938117 569027612 902394793 "
        "1019770837 221470752 669768613 1839284764 1979413630 1335703733 "
        "1526078440 1403144959 1139398206 753967943 1785700701 1187714882 "
        "1063522909 1123137582 192083544 680202567 1109090588 327456556 "
        "1709233078 191596027 1076438936 1306955024 1530346852 127901445 "
        "8455468 377129974 1199230721 1336700752 1103107597 703058228 "
        "844612202 530372344 1910850558 47387421 1871435357 1168551137 "
        "1101007744 1918050856 803711675 309982095 73743043 301259382 "
        "1647477295 1644236294 859823662 638826571 1487427444 335916581 "
        "15468904 140348241 895842081 410006250 1847504174 536600445 "
        "1359845362 1400027760 288242141 1910039802 1453396858 1761991428 "
        "2137921913 357210187 1414819544 1933136424 943782705 841706193 "
        "1081202962 1919045067 333546776 988345562 337850989 314809455 "
        "1750287624 853099962 1450233962 142805884 1399258689 247367726 "
        "2128513937 1151147433 654730608 351121428 12778440 18876380 "
        "1575222551 587014441 411835569 380613902 1771550148";
    std::knuth_b e1(10);
    std::ostringstream os;
    os << e1;
    assert(os.str() == a);
}

int main()
{
    test1();
}
