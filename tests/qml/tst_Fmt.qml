import QtQuick
import QtTest
import FarmaciaSim

TestCase {
    name: "Fmt"

    function test_eur() {
        compare(Fmt.eur(1234567), "1.234.567 €")
        compare(Fmt.eur(0), "0 €")
    }

    function test_eur2() {
        compare(Fmt.eur2(1234.5), "1.234,50 €")
    }

    function test_pct_defaultDecimal() {
        compare(Fmt.pct(0.025), "2,5 %")
    }

    function test_pct_explicitDecimals() {
        compare(Fmt.pct(0.0256, 2), "2,56 %")
        compare(Fmt.pct(0.03, 0), "3 %")
    }

    function test_num() {
        compare(Fmt.num(1234567), "1.234.567")
        compare(Fmt.num(1234.5, 1), "1.234,5")
    }

    function test_parse_roundTripsWithEur() {
        compare(Fmt.parse(Fmt.eur(1234567)), 1234567)
    }

    function test_parse_handlesEsEsThousandsAndDecimal() {
        compare(Fmt.parse("1.234,56 €"), 1234.56)
        compare(Fmt.parse("2,5 %"), 2.5)
        compare(Fmt.parse("-45"), -45)
    }

    function test_byFmt() {
        compare(Fmt.byFmt(0.025, "pct1"), Fmt.pct(0.025, 1))
        compare(Fmt.byFmt(0.025, "pct2"), Fmt.pct(0.025, 2))
        compare(Fmt.byFmt(3, "num"), Fmt.num(3, 1))
        compare(Fmt.byFmt(1000, "eur"), Fmt.eur(1000))
    }
}
