# HARD VOX v1.0 — Plugin VST3 para FL Studio (Windows 10 / 11)

Procesador vocal hardtrap: Compresor, EQ, Brillo/Aire, Distorsión/Saturación, Chorus, Reverb, Delay,
Stereo Widener y Ganancia, con medidores de entrada y salida, 9 presets y cadena de efectos personalizable.

**No necesitas programar ni instalar Visual Studio.** GitHub compila el plugin por ti, gratis.

---

## PARTE 1 — Subir el proyecto a GitHub (una sola vez)

1. **Crea una cuenta** en https://github.com (botón *Sign up*). Si ya tienes, inicia sesión.
2. **Descomprime** `HARD_VOX_GITHUB_READY.zip` en una carpeta de tu PC (clic derecho → *Extraer todo*).
   Dentro verás: `CMakeLists.txt`, `README.md`, y las carpetas `Source`, `Assets` y `.github`.
3. En GitHub, arriba a la derecha pulsa **+** → **New repository**.
   - *Repository name*: `HARD-VOX`
   - Marca **Public** (las compilaciones son gratis e ilimitadas en repos públicos).
   - Pulsa **Create repository**.
4. En la página del repositorio nuevo pulsa el enlace **uploading an existing file**
   (o **Add file → Upload files**).
5. Abre la carpeta que descomprimiste, **selecciona TODO su contenido** (Ctrl + A) y **arrástralo**
   a la ventana del navegador. Espera a que termine de subir.
   - Importante: tienen que subirse también las carpetas `.github`, `Source` y `Assets`.
6. Abajo pulsa el botón verde **Commit changes**.

> Comprobación rápida: en la página principal del repositorio debes ver `.github`, `Assets`, `Source`,
> `CMakeLists.txt` y `README.md`. Si falta `.github`, entra en **Add file → Upload files** y arrastra esa carpeta.

## PARTE 2 — Esperar la compilación automática

1. En tu repositorio pulsa la pestaña **Actions** (arriba).
2. Verás una ejecución llamada **Compilar HARD VOX (VST3 para Windows)**.
   - 🟡 Círculo amarillo = compilando (tarda unos **10–20 minutos** la primera vez).
   - ✅ Check verde = terminó bien.
   - ❌ X roja = falló (mira la sección *Si algo falla*).
3. Si no arrancó sola: Actions → *Compilar HARD VOX* (columna izquierda) → **Run workflow** → **Run workflow**.

## PARTE 3 — Descargar tu VST3

1. En **Actions**, pulsa la ejecución con el ✅ verde.
2. Baja hasta la sección **Artifacts** (al final de la página).
3. Descarga **HARD_VOX_VST3** (es un .zip).
4. Descomprímelo: dentro hay una carpeta llamada **`HARD VOX.vst3`**.

> Si Windows bloquea el archivo: clic derecho sobre el .zip descargado → *Propiedades* → marca **Desbloquear** → Aceptar,
> y luego descomprime.

## PARTE 4 — Instalar en Windows

1. Abre el Explorador de archivos y ve a: `C:\Program Files\Common Files\VST3`
   (si la carpeta `VST3` no existe, créala).
2. Copia ahí la carpeta **`HARD VOX.vst3`** completa (Windows pedirá permiso de administrador: pulsa *Continuar*).

## PARTE 5 — Que FL Studio lo reconozca

1. Abre FL Studio → menú **Options → Manage plugins**.
2. Comprueba que en *Plugin search paths* aparece `C:\Program Files\Common Files\VST3`
   (si no, añádela con el icono de carpeta).
3. Pulsa **Find installed plugins** (o *Start scan*) y espera.
4. Para usarlo en la voz: en el **Mixer**, haz clic en un hueco de efectos (*Slot*) → busca **HARD VOX** en la lista
   (si no sale, abre el menú *More plugins* o escribe "HARD" en el buscador).
5. Si no aparece: en *Manage plugins* activa **Verify plugins** y vuelve a escanear; ciérralo y ábrelo de nuevo.

---

## Cómo se usa

- **Perillas**: arrastra arriba/abajo. `Shift` = ajuste fino. **Doble clic** = valor por defecto. Rueda del ratón también funciona.
- **Flechas ◀ ▶** y clic en el nombre del preset (arriba): cambiar preset / menú de presets.
- **Gráfico del EQ**: arrastra los 4 puntos (arriba/abajo = ganancia, izquierda/derecha = frecuencia; el punto de AIRE solo ganancia).
- **Botón CONTROLES** (arriba a la derecha): panel con TODOS los parámetros (Attack, Release, Saturation, Size, Feedback,
  Mono compat., etc.), interruptor ON/OFF por módulo, y botones **Reset Default / Guardar / Cargar / Borrar preset**.
- **Botón CADENA**: arrastra los bloques para cambiar el orden de los efectos. **ORDEN ESTANDAR** vuelve al orden recomendado:
  Compresor → EQ → Brillo/Aire → Distorsión → Chorus → Reverb → Delay → Stereo Widener → Ganancia de salida.
- **Presets propios**: se guardan en `%APPDATA%\HardVox\Presets` (siguen ahí al reiniciar FL Studio).
- Todos los parámetros son automatizables desde FL Studio.
- Ventana redimensionable (arrastra la esquina).

### Qué hace cada perilla grande / pequeña de la interfaz
| Módulo | Perilla grande | Perillas pequeñas / etiquetas |
|---|---|---|
| Compressor | Makeup | THRESH, RATIO |
| EQ | — | LOW, MID, HIGH (+ gráfico con 4 puntos) |
| Brillo/Aire | Brillo | AIR |
| Distorsión | Saturación | DRIVE |
| Reverb | Decay | MIX |
| Delay | Feedback | TIME |
| Chorus | Mix | DEPTH |
| Stereo Widener | Stereo Amount | WIDTH |
| Gain | perilla y barra = ganancia master | |

---

## Si algo falla

- **Falla la compilación (❌)**: entra a la ejecución → pulsa el paso rojo → copia las últimas ~30 líneas del error
  y envíaselas a Claude para que lo corrija. (El código no pudo probarse en un compilador real antes de entregarlo.)
- **FL Studio no lo ve**: revisa que la carpeta `HARD VOX.vst3` esté completa (con su carpeta `Contents` dentro)
  y repite el escaneo de plugins.
- **Si `windows-2022` deja de existir en GitHub** (los runners se retiran con los años): abre `.github/workflows/build.yml`
  en GitHub (lápiz de editar) y cambia `windows-2022` por `windows-latest`.

## Notas legales
- Usa el framework JUCE 7 (se descarga solo). Para uso personal no hay problema; si quieres **distribuir o vender** el plugin,
  JUCE exige licencia comercial o publicar el código bajo GPL/AGPL.
- Los nombres de artistas en los presets/imagen son solo referencias de estilo; si lo publicas, evita sugerir afiliación.

## Estructura del proyecto
```
CMakeLists.txt            configuración de compilación (descarga JUCE sola)
.github/workflows/build.yml   compilación automática en Windows
Assets/background.png     interfaz (se incrusta dentro del plugin)
Source/Params.h           definición de todos los parámetros + orden de cadena
Source/Dsp.h              módulos de audio (compresor, EQ, brillo, distorsión, chorus, reverb, delay, widener)
Source/PluginProcessor.*  motor de audio, cadena personalizable, presets de fábrica y de usuario
Source/PluginEditor.*     interfaz, medidores, animaciones de personajes, paneles
```
