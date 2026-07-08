# FarmaciaSim en el navegador (WebAssembly)

La misma app compilada a WebAssembly corre en cualquier navegador moderno,
incluido Safari de iPad: se abre la URL, "Añadir a pantalla de inicio", y se
comporta como una app con icono propio. Los datos se guardan en el
`localStorage` del navegador (cada dispositivo recuerda los suyos).

## Vía A (recomendada): GitHub Actions, sin instalar nada

1. Crea un repositorio en GitHub y sube el contenido de esta carpeta
   (incluido `.github/workflows/wasm.yml`).
2. En el repositorio: **Settings → Pages → Source: "GitHub Actions"**.
3. Cada `git push` compila y publica automáticamente. La URL queda en
   `https://<tu-usuario>.github.io/<repo>/`.

Con repositorio privado también funciona (GitHub da minutos gratuitos de
sobra para esto), pero la página publicada será visible para quien tenga
la URL.

## Vía B: compilar en tu PC con Windows

1. **Qt para WebAssembly**: abre *Qt Maintenance Tool* → Add or remove
   components → Qt 6.11.1 → marca **WebAssembly (single-threaded)**.
2. **Emscripten 4.0.7** (la versión exacta que exige Qt 6.11):
   ```bat
   git clone https://github.com/emscripten-core/emsdk C:\emsdk
   cd C:\emsdk
   emsdk install 4.0.7
   emsdk activate 4.0.7 --permanent
   ```
3. En Qt Creator aparecerá el kit *WebAssembly Qt 6.11.1*; si pide la ruta
   de emscripten: `C:\emsdk`. Selecciona ese kit y compila en Release.
4. Probar en local (no vale abrir el .html a pelo, hace falta un servidor):
   ```bat
   cd carpeta-de-build
   python -m http.server 8000
   ```
   y abre `http://localhost:8000/FarmaciaSim.html`.
5. Para usarlo desde el iPad: sube `FarmaciaSim.html` (renombrado a
   `index.html`), `FarmaciaSim.js`, `FarmaciaSim.wasm` y `qtloader.js` a
   cualquier hosting estático (GitHub Pages, Netlify, tu propio dominio…).

## Qué esperar

- Primera carga: descarga ~25-40 MB (luego el navegador lo cachea y abre
  rápido). Rendimiento sobrado para esta app.
- Persistencia: automática en `localStorage`, por navegador y dispositivo.
  Si se borran los datos de navegación, vuelve a los valores del Excel.
- La interfaz es la de escritorio; funciona con el dedo, pero los campos
  son pequeños para tablet. Si el uso en iPad se vuelve habitual, merece
  una pasada de adaptación táctil (campos más grandes, barra colapsable).
