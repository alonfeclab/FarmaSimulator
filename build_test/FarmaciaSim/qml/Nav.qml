pragma Singleton
import QtQuick

// Coordina la navegación entre pestañas: permite pedir "ve a la hoja X
// y enfoca el campo Y" desde cualquier vista sin conocer el StackLayout.
QtObject {
    signal irA(int indice, string foco)
}
