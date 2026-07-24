pragma Singleton
import QtQuick
import FarmaciaSim

// Tokens de color de la app, con soporte de tema claro/oscuro: cada token
// es un color derivado de "dark" (ver botón sol/luna en NavPanel.qml), así
// que cambiar "dark" repinta toda la app al vuelo sin tocar ninguna vista
// (todas usan Tokens.xxx, nunca colores sueltos). "dark" sigue en vivo a
// Engine.darkTheme (persistido en disco, ver Engine::setDarkTheme); el botón
// sol/luna llama a Engine.setDarkTheme() en vez de tocar esta propiedad
// directamente, para no romper el binding.
// Un mismo color hexadecimal puede aparecer en varios tokens cuando cumple
// roles de diseño distintos (p.ej. blanco de tarjeta vs. blanco de fila de
// tabla): eso es intencionado, no duplicación. Ver qml/design-tokens.json
// para el catálogo completo (valores del tema claro).
QtObject {
    id: tokens

    property bool dark: Engine.darkTheme

    readonly property color bgApp: dark ? "#101915" : "#eef2f0"
    readonly property color bgNav: dark ? "#0b2018" : "#123f31"
    readonly property color bgNavItemActive: dark ? "#21a37d" : "#1a7a5e"
    readonly property color bgNavItemHover: dark ? "#17392c" : "#1a5a45"
    readonly property color bgSurface: dark ? "#17241e" : "#ffffff"
    readonly property color bgBrandStrong: dark ? "#1f6b52" : "#14523f"
    readonly property color bgTableRowPrimary: dark ? "#17241e" : "#ffffff"
    readonly property color bgTableRowAlt: dark ? "#1c2b24" : "#f7faf8"
    readonly property color bgTableRowEmphasis: dark ? "#234133" : "#e3efe9"
    readonly property color bgTableRowSeparator: dark ? "#24352c" : "#dde9e2"
    readonly property color bgAccentTint: dark ? "#1b3229" : "#eef5f1"
    readonly property color bgSelection: dark ? "#0c4a38" : "#1a7a5e"
    readonly property color bgInput: dark ? "#332c19" : "#fffbe8"
    readonly property color bgInputInvalid: dark ? "#3a211d" : "#fdecea"
    readonly property color bgInputPressed: dark ? "#4a3f24" : "#e0d6ac"
    readonly property color bgDropdownItemHighlighted: dark ? "#3d3d20" : "#eef0c9"
    readonly property color bgRowCardDefault: dark ? "#1b2a23" : "#f4f8f6"
    readonly property color bgRowCardEmphasis: dark ? "#2a5443" : "#bfe2cf"
    readonly property color bgButtonPrimaryDefault: dark ? "#21a37d" : "#1a7a5e"
    readonly property color bgButtonPrimaryPressed: dark ? "#16543f" : "#0f5a43"
    readonly property color bgButtonSecondaryDefault: dark ? "#1d4536" : "#1a5a45"
    readonly property color bgButtonSecondaryPressed: dark ? "#0a2019" : "#0e3226"
    readonly property color bgButtonGhostDefault: dark ? "#17241e" : "#ffffff"
    readonly property color bgButtonDangerDefault: dark ? "#c4493a" : "#a33b2e"
    readonly property color bgButtonDangerPressed: dark ? "#9c392b" : "#8a2a1f"
    readonly property color bgSwitchTrackOn: dark ? "#21a37d" : "#1a7a5e"
    readonly property color bgSwitchTrackOff: dark ? "#33413a" : "#c9d6cf"
    readonly property color bgSwitchThumb: dark ? "#eef1ef" : "#ffffff"
    readonly property color bgTransparent: "transparent"

    readonly property color textOnDark: "#ffffff"
    readonly property color textOnDarkMuted: dark ? "#8fc0ae" : "#7fae9c"
    readonly property color textOnDarkSecondary: dark ? "#bfe0d2" : "#cfe8de"
    readonly property color textButtonPrimary: "#ffe9a8"
    readonly property color textHeading: dark ? "#4fcf9c" : "#14523f"
    readonly property color textSecondary: dark ? "#c3cdc8" : "#3c4a46"
    readonly property color textPrimary: dark ? "#e7ece9" : "#1e2b28"
    readonly property color textNegative: dark ? "#e5695a" : "#a33b2e"
    readonly property color textMuted: dark ? "#93a19c" : "#6b7a76"
    readonly property color textFaint: dark ? "#8a9793" : "#7a8985"
    readonly property color textInvalid: dark ? "#e2584a" : "#c0392b"
    readonly property color textWarning: dark ? "#e0952e" : "#a15c00"
    readonly property color textButtonGhost: dark ? "#6fcf9f" : "#1a5a45"

    readonly property color borderSurface: dark ? "#2c3b34" : "#dde5e1"
    readonly property color borderDivider: dark ? "#26342e" : "#dde5e1"
    readonly property color borderNavDivider: dark ? "#2f7057" : "#2b6a52"
    readonly property color borderButtonPrimary: dark ? "#34a37e" : "#2b8a6a"
    readonly property color borderButtonSecondary: dark ? "#2f7057" : "#2b6a52"
    readonly property color borderButtonDanger: dark ? "#d97a67" : "#c24d3c"
    readonly property color borderButtonGhostDefault: dark ? "#37453e" : "#c9d6d0"
    readonly property color borderInteractiveHover: dark ? "#21a37d" : "#1a7a5e"
    readonly property color borderInputDefault: dark ? "#5c4f2e" : "#e0d6ac"
    readonly property color borderInputInvalid: dark ? "#e2584a" : "#c0392b"
    readonly property color borderTableGroupEnd: dark ? "#4a6358" : "#8fb3a3"
    readonly property color borderRowCardDefault: dark ? "#2e463a" : "#d5e6dc"
    readonly property color borderRowCardEmphasis: dark ? "#3f8064" : "#7fbb9b"
    readonly property color borderAccent: dark ? "#2ecf9e" : "#1a7a5e"
    readonly property color borderSwitchTrackOff: dark ? "#4a5951" : "#a8b8b0"

    readonly property color iconOnDark: "#ffffff"
    readonly property color iconAccent: "#ffe9a8"
}
