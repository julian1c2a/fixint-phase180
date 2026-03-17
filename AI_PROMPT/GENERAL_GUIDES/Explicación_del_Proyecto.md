# QUÉ SEA ESTE PROYECTO (What would this project be?) #

## OBJETIVO 1

Proporcionar un tipo entero de 128 bits que sea similiar a los tipos enteros nativos de determinados por el lenguaje C++ y C, en términos de funcionalidad y estándar del lenguaje C++, además de conseguir rendimiento aceptables. Comparamos estos rendimienhtos con los conseguidos por los tipos enteros nativos de 64 bits y 32 bits, por los tipos __int128_t__ y __uint128_t__ de GCC y Clang, y por las bibliotecas de enteros grandes como GMP, TomMath y Boost.Multiprecision.

## OBJETIVO 2

El tipo entero de 128 bits deber ser absolutamente portable, es decir, debe compilar y funcionar correctamente en cualquier plataforma y con cualquier compilador que soporte el estándar C++20 o superior. Ha de ser tan portable que debemos poder compilarlo y usarlo en plataformas y compiladores que no soporten los tipos __int128_t__ y __uint128_t__ nativos de 128 bits, como MSVC, aún más, en plataformas de 32 bits, aún más, en un etapa final hay que correrlo en plataformas ARM64, ARM32, RISCV32 Y RISCV64. Para ello tenemos ya la infraestructura de preprocesador necesaria para detectar las características del compilador y la plataforma en tiempo de compilación, y así adaptar el código a las mismas.

## OBJETIVO 3

El usuario de la biblioteca debe sentirla como parte del lenguaje C++, sin notar diferencias entre el uso de los tipos enteros nativos y el uso del tipo entero de 128 bits proporcionado por esta biblioteca. Cuando esta biblioteca se use en un proyecto, el usuario no debe tener que aprender nada nuevo para usar el tipo entero de 128 bits, ya que la interfaz y el comportamiento deben ser idénticos a los de los tipos enteros nativos de C++. Cunado esta biblioteca (en principio solo cabeceras) incluya fixed_int, y otros tipos de longitudes mayores han de ser meras extensiones de estos tipos enteros de 128 bits. Por lo tanto, la integración con el ecosistema C++, librerías estándar y de terceros, debe ser una prioridad en el diseño y la implementación del tipo entero de 128 bits.

## OBJETIVO 4

Inicialmente, los compiladores C++ serán gcc-13, gcc-14, gcc-15 en WSL (Ubuntu 25.04), clang-18, clang-19, clang-20 y clang-21 en WSL (Ubuntu 25.04), oneAPI de Intel (icpx) en WSL (Ubuntu 25.04), MSVC 2026 en Windows 11, y Intel C++ Compiler (icx) en Windows 11. El sistema de build debe de hacer un ciclo completo de CI/CD, incluyendo compilación de los tests, compilación de los benchmarks, ejecución de los tests y ejecución de los benchmarks, todo ello de forma automática y sin intervención del usuario.

## OBJETIVO 5

El proyecto debe incorporar los sanitizadores, los analizadores estáticos de código y las herramientas de cobertura de código más relevantes y populares, como AddressSanitizer, UndefinedBehaviorSanitizer, Clang-Tidy, Cppcheck, GCov y LCOV. Estas herramientas deben integrarse en el sistema de construcción del proyecto, de manera que se puedan ejecutar fácilmente durante el proceso de compilación y prueba. El uso de estas herramientas ayudará a detectar errores y problemas potenciales en el código, mejorando así la calidad y la fiabilidad del tipo entero de 128 bits proporcionado por la biblioteca.

## OBJETIVO 6

Por el punto anterior, el proyecto debe incluir un conjunto de pruebas unitarias y benchmarks que verifiquen la corrección y el rendimiento de los tipos que se definan en la biblioteca. Cada compilación ha de hacerse siguiendo una normativa clara y precisa, de manera que se puedan reproducir los resultados obtenidos en las pruebas y benchmarks en cualquier plataforma y con cualquier compilador soportado. Después de cada aditamento al código, se deben ejecutar todas las pruebas y benchmarks para asegurar que no se han introducido errores o regresiones en el rendimiento.

## OBJETIVO 7

El proyecto debe tener una estructura muy clara y organizada, con una separación adecuada entre el código fuente, las pruebas unitarias, los benchmarks, la documentación para usuarios de la biblioteca, de la que es para el mantenedor (o codificador de la misma), incluso de la que es para la IA que esté actuando. Si se necesitan hacer pruebas se generará un directorio debugging, este tendrá en su interior los directorios src y build. En el directorio raiz no se harán pruebas, y se asumirá que todo se hace desde el raíz del proyecto sin necesidad de entrar en subdirectorios.

## OBJETIVO 8

El sistema de construcción del proyecto recae sobre scripts python (en scripts/) que proporcionan con facilidad entornos de variables de entorno locales y temporales (mientras dura la compilación) para cada compilador y plataforma soportada. Los demás scripts deben recaer sobre estos (llamadas a estos scripts principales). El uso de mske y makefiles quedará condicionado a usar estos scripts python. A su vez, la gran estructura CMake quedará condicionada a usar el nivel de make/makefiles. Usaremos CMake/CTest. Nos falta definir el script CI/CD, que pueda ser llamado desde GitHub Actions u otros sistemas CI/CD (incluso a voluntad).

## OBJETIVO 9

El tipo entero de 128 bits debe ser fácil de usar, es decir, debe proporcionar una interfaz sencilla y clara para los usuarios, similar a la de los tipos enteros nativos de C++. Debe soportar todas las operaciones aritméticas, lógicas y de comparación habituales, así como conversiones entre tipos y operaciones de entrada/salida. De hecho no debe haber ninguna diferencia en el uso entre este tipo y los tipos enteros nativos de C++. Integración absoluta con la STL y otras librerías estándar del ecosistema C++ y de terceros.

## OBJETIVO 10

Este proyecto debe ser una primera etapa para la creación de una biblioteca más amplia de tipos enteros de precisión arbitraria, que puedan ser utilizados en aplicaciones que requieran cálculos con números muy grandes, como criptografía, simulaciones científicas, etc. Por lo tanto, el diseño del tipo entero de 128 bits debe ser modular y extensible, de manera que se pueda ampliar fácilmente en el futuro, de ahí la preocupación por introducir las representaciones Magnitud&Signo y Excceso-K para soportar tipos punto flotante (IEEE 754 Generalizado). También tendremos implementaciones del mismo tipo para punto fijo dónde la longitud de la parte entera y la parte fraccionaria pueda ser configurable por parámetros de plantilla. Tambíén tendremos implementaciones de racionales exactos.

## OBJETIVO 11

Este proyecto tiene una especial atención al aprendizaje de los distintos algoritmos que mejoran el rendimiento de las operaciones aritméticas con enteros grandes, como la división knuth_D, la multiplicación de Karatsuba, etc. Por lo tanto, el código debe estar bien documentado y comentado, explicando los algoritmos utilizados y las decisiones de diseño tomadas. Además, se deben incluir pruebas unitarias y benchmarks para verificar la corrección y el rendimiento del tipo entero de 128 bits en comparación con otros tipos y bibliotecas existentes. Tomamos ideas de codificación de bibliotecas como GMP, TomMath, Boost.Multiprecision, etc.

## OBJETIVO 12

El proyecto debe proporcionar unos tipos simétricos a los enteros, solo que en vez de en base 2, en base 10, es decir, tipos decimales de 128 bits y superiores. Estos tipos decimales deben ser compatibles con los tipos enteros de 128 bits y superiores, y deben proporcionar una interfaz similar a la de los tipos enteros nativos de C++. Estos tipos decimales serán implementados en una fase posterior del proyecto, una vez que los tipos enteros estén completamente implementados y probados. En principio estos tipos no serán tan estándar, no implementarán DCD, hemos de codificar un tipo decimal simple (1 dígito base 10 por byte) Usaremos BCD Natural (1 dígito decimal por byte), para los tipos sin signo. Para los tipos con signo en complemento a la base usaremos BCD Aiken (2-4-2-1). La idea es dar finalmente un conjunto de tipo gemelos a los anteriores pero con representación en base 10 nativa. (Por comenzar)

# ETAPAS A CUBRIR DEL PROYECTO (STAGES TO COVER IN THE PROJECT) #

## ETAPA 1

Implementación básica del tipo entero de 128 bits, con soporte para operaciones aritméticas básicas (suma, resta, multiplicación, división) y conversiones entre tipos. Implementación de pruebas unitarias y benchmarks para verificar la corrección y el rendimiento del tipo entero de 128 bits en comparación con los tipos enteros nativos de C++ y las bibliotecas existentes. Los tipo enteros iniciales serán int128_t y uint128_t (tipos en headers aparte), más todo el conjunto de headers y código necesario para que el proyecto funcione correctamente. (Terminado)

## ETAPA 2

Unificación de las distintas implmentaciones de 128 bits en una sola implementación que use plantillas para seleccionar la representación interna (siempre la misma, común, array de uint64_t, etc.). Un solo header, a la plantilla se le pasa un parámetro que indica que es con signo o sin signo. Implementación de pruebas unitarias y benchmarks para verificar la corrección y el rendimiento del tipo entero de 128 bits unificado en comparación con los tipos enteros nativos de C++ y las bibliotecas existentes. (Terminado)

## ETAPA 3

Implementación de nuevas formas de representar enteros de 128 bits con signo, como Magnitud&Signo y Exceso-K. Estos nuevos tipos tienen interés para la futura implementación de tipos punto flotante (IEEE 754 Generalizado). Implementación de pruebas unitarias y benchmarks para verificar la corrección y el rendimiento de las nuevas representaciones en comparación con la representación por defecto (complemento a dos). (En progreso) (M&S casi terminado, Exceso-K pendiente) 35/12/20/01/2026 (MM/HH/DD/MM/YYYY)

## ETAPA 4

Implementación de extensiones completas para trabajar en en vez de 128 bits con arrays de N>0 elementos de uint64_t (por comenzar, etapa 1.80). Implementación de pruebas unitarias y benchmarks para verificar la corrección y el rendimiento de los tipos enteros de N*64 bits en comparación con los tipos enteros nativos de C++ y las bibliotecas existentes. Solo tipos enteros. (Por comenzar)

## ETAPA 5

Implementación de tipos de punto fijo basados en los tipos enteros de N*64 bits, donde la longitud de la parte entera se llamará E y la de la parte fraccionaria será F, pueda ser configurable por parámetros de plantilla. Se implementarán solo 2 modalidades: signed y unsigned, siendo la signed en complemento a 2 (únicamente). Implementación de pruebas unitarias y benchmarks para verificar la corrección y el rendimiento de los tipos de punto fijo en comparación con los tipos enteros nativos de C++ y las bibliotecas existentes. (Por comenzar)

## ETAPA 6

Implementación de los tipos en punto flotante basados en la representación IEEE 754 Generalizado. Se implementarán basadas en los tipos que ya tenemos: M&S de N*64 bits para la mantisa, Exc-K para el exponente. Lo que habrá que ver es un un storage que cargue un signo (para magnitud y signo) , posteriomente el exponente y finalmente la magnitud de la mantisa. Implementación de pruebas unitarias y benchmarks para verificar la corrección y el rendimiento de los tipos de punto flotante en comparación con los tipos de punto flotante nativos de C++ y las bibliotecas existentes. (Por comenzar)

## ETAPA 7

Implmentación de tipos enteros de longitud arbitraria (big integers) basados en el tipo std::string_base<uint64_t>, o alguno que nosotros implmentemos similar. Además, implementaremos solo tipos signos en Complemento a 2. Implementación de pruebas unitarias y benchmarks para verificar la corrección y el rendimiento de los tipos enteros de longitud arbitraria en comparación con las bibliotecas existentes como GMP, TomMath y Boost.Multiprecision. (Por comenzar)

## ETAPA 8

Implementación de tipos racionales exactos basados en los tipos enteros de longitud arbitraria. Implementación de pruebas unitarias y benchmarks para verificar la corrección y el rendimiento de los tipos racionales exactos en comparación con las bibliotecas existentes como GMP, TomMath y Boost.Multiprecision. (Por comenzar)

## ETAPA 9

Implementación de tipos decimales de 128 bits y superiores, basados en representaciones en base 10 (BCD Natural para sin signo, BCD Aiken para con signo). Implementación de pruebas unitarias y benchmarks para verificar la corrección y el rendimiento de los tipos decimales en comparación con los tipos enteros de 128 bits y superiores. (Por comenzar)
