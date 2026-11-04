#include <assert.h>
#include <math.h>
#include <string.h>
#include <map>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <float.h>
#include "../src/Registro/RelojCore.h"
#include "../src/Registro/Resumen.h"
#include "../src/Registro/Archivo.h"
#include "../src/Registro/CSV.h"
#include "../src/Registro/ConfigRegistro.h"
#include "../src/Sensores/Energia/SesionEnergia.h"
struct Disco {
    std::map<std::string,std::string> archivos;
    std::string actual;
    bool disponible=true, fallaRenombrar=false, fallaReabrir=false;
    size_t limitar=SIZE_MAX;
    unsigned flushes=0;
};
bool existe(void* x,const char* p) { return static_cast<Disco*>(x)->archivos.count(p); }
bool abrir(void* x,const char* p,bool append) {
    auto& d=*static_cast<Disco*>(x);
    if (!d.disponible || (append && d.fallaReabrir)) return false;
    d.actual=p; if (!append) d.archivos[p].clear(); return true;
}
size_t escribir(void* x,const char* s,size_t n) {
    auto& d=*static_cast<Disco*>(x); n=std::min(n,d.limitar);
    d.archivos[d.actual].append(s,n); return n;
}
void sincronizar(void* x) { ++static_cast<Disco*>(x)->flushes; }
bool error(void*) { return false; }
void cerrar(void* x) { static_cast<Disco*>(x)->actual.clear(); }
bool renombrar(void* x,const char* a,const char* b) {
    auto& d=*static_cast<Disco*>(x);
    if (d.fallaRenombrar || d.archivos.count(b)) return false;
    d.archivos[b]=d.archivos.at(a); d.archivos.erase(a); return true;
}
std::vector<std::string> dividir(const std::string& s) {
    std::vector<std::string> r; size_t a=0;
    for (size_t i=0;i<s.size();++i) if (s[i]==',' || s[i]=='\n') { r.push_back(s.substr(a,i-a)); a=i+1; }
    return r;
}
int main(int argc,char** argv) {
    int64_t ms;
    assert(convertirUnix({2024,3,1,0,0,0,0},ms) && ms==1709251200000LL);
    char f[16],h[20]; formatearArgentina(ms,f,sizeof(f),h,sizeof(h));
    assert(std::string(f)=="29/02/2024" && std::string(h)=="21:00:00.000");
    assert(convertirUnix({2026,1,1,1,15,59,123},ms));
    formatearArgentina(ms,f,sizeof(f),h,sizeof(h));
    assert(std::string(f)=="31/12/2025" && std::string(h)=="22:15:59.123");
    assert(!convertirUnix({2023,2,29,0,0,0,0},ms));
    assert(!convertirUnix({2026,13,1,0,0,0,0},ms));
    assert(!convertirUnix({2026,1,1,24,0,0,0},ms));
    assert(convertirUnix({2024,2,29,2,30,0,0},ms));
    formatearArgentina(ms,f,sizeof(f),h,sizeof(h)); assert(std::string(f)=="28/02/2024");
    ReferenciaUTC reloj;
    assert(estimarHora(reloj,1000000,3000).fuente==FuenteHora::SIN_HORA);
    aplicarSincronizacion(reloj,ms,1000000);
    assert(estimarHora(reloj,3000000,3000).fuente==FuenteHora::GPS);
    assert(estimarHora(reloj,6000000,3000).fuente==FuenteHora::ESTIMADA);
    assert(estimarHora(reloj,6000000,3000).unixMs==ms+5000);
    assert(aplicarSincronizacion(reloj,ms+4000,6000000)==-1000);
    assert(estimarHora(reloj,6000000,3000).edadMs==0);
    // El GPS puede actualizarse entre tomar una foto y buscarle hora.
    // Una referencia posterior a la foto no debe borrar una hora anterior válida.
    ReferenciaUTC actualizada=reloj;
    const ReferenciaUTC antesDeCapturar=reloj;
    aplicarSincronizacion(actualizada,ms+5000,7000001);
    assert(estimarHora(actualizada,7000000,3000).fuente==FuenteHora::SIN_HORA);
    assert(estimarHora(antesDeCapturar,7000000,3000).fuente==FuenteHora::GPS);
    const ReferenciaUTC sinHora;
    assert(estimarHora(sinHora,7000000,3000).fuente==FuenteHora::SIN_HORA);

    AcumuladorElectrico a;
    actualizarResumen(a,1000000,2,96,true,2000000);
    actualizarResumen(a,1250000,6,312,true,2250000);
    ResumenElectrico r=cerrarResumen(a,2000000);
    assert(r.coberturaUs==1000000 && fabs(r.corrienteMedia-5)<1e-9 && fabs(r.potenciaMedia-258)<1e-9 && r.corrienteMax==6);
    a=AcumuladorElectrico{};
    actualizarResumen(a,1000000,8,384,true,1300000);
    r=cerrarResumen(a,2000000);
    assert(r.coberturaUs==300000 && r.ventanaUs==1000000 && r.corrienteMedia==8);
    r=cerrarResumen(a,3000000); assert(r.coberturaUs==0);
    a=AcumuladorElectrico{};
    actualizarResumen(a,0,-2,-96,true,2000000);
    r=cerrarResumen(a,1000000); assert(r.corrienteMax==-2 && r.corrienteMedia==-2);

    Disco d; BackendArchivo b={&d,existe,abrir,escribir,sincronizar,error,cerrar,renombrar};
    ArchivoRegistro archivo;
    d.disponible=false; assert(!crearArchivoRegistro(archivo,b,0,0,true,"a,b\n"));
    d.disponible=true; assert(crearArchivoRegistro(archivo,b,0,0,true,"a,b\n"));
    std::string original=archivo.ruta;
    assert(escribirFilaArchivo(archivo,b,"1,2\n",4));
    assert(convertirUnix({2026,10,14,17,0,35,0},ms));
    assert(fecharArchivo(archivo,b,ms,0,true));
    assert(std::string(archivo.ruta).find("2026-10-14_14-00-35")!=std::string::npos);
    assert(d.archivos.at(archivo.ruta)=="a,b\n1,2\n" && !d.archivos.count(original));
    d.fallaRenombrar=true; original=archivo.ruta;
    assert(!fecharArchivo(archivo,b,ms,0,true) && archivo.abierto && original==archivo.ruta);
    assert(escribirFilaArchivo(archivo,b,"3,4\n",4));
    d.limitar=2; assert(!escribirFilaArchivo(archivo,b,"5,6\n",4));
    assert(archivo.incierto && !archivo.abierto);
    assert(!escribirFilaArchivo(archivo,b,"7,8\n",4));
    d.limitar=SIZE_MAX; d.fallaRenombrar=false;
    assert(crearArchivoRegistro(archivo,b,ms,1,true,"a,b\n"));
    assert(std::string(archivo.ruta)!=original && d.archivos.at(original)=="a,b\n1,2\n3,4\n5,");
    assert(cerrarArchivo(archivo,b));
    assert(crearArchivoRegistro(archivo,b,0,0,false,"a,b\n"));
    assert(std::string(archivo.ruta)=="/R0000001.CSV");
    assert(cerrarArchivo(archivo,b));
    assert(crearArchivoRegistro(archivo,b,0,0,false,"a,b\n") && std::string(archivo.ruta)=="/R0000002.CSV");

    SesionEnergia sesion(17,48);
    sesion.iniciar(0); sesion.actualizar(48,8.5,0,true);
    const EstadoEnergia e=sesion.actualizar(48,8.5,3600000,true);
    assert(e.potenciaW==408 && fabs(e.ahConsumidos-8.5)<1e-5 && fabs(e.whConsumidos-408)<1e-5 && e.porcentajeBateria==50);
    sesion.pausar(3600000); sesion.actualizar(48,8.5,7200000,true);
    assert(sesion.segundosIntegrados()==3600 && sesion.obtenerEstado().porcentajeBateria==50);
    sesion.reiniciar(7200000); assert(sesion.segundosIntegrados()==0 && sesion.obtenerEstado().porcentajeBateria==100);

    RegistroFila fila={}; strcpy(fila.sessionId,"SIMULADO_NO_MEDICION_REAL");
    fila.snapshot.capturaUs=1000000;
    char buffer[RegistroConfig::MAX_CSV];
    size_t n=formatearCSV(fila,buffer,sizeof(buffer)); assert(n>0 && buffer[n-1]=='\n');
    const auto encabezado=dividir(cabeceraCSV()); auto c=dividir(buffer);
    assert(encabezado.size()==70 && c.size()==encabezado.size());
    assert(encabezado.back()=="registros_perdidos" && c[0]=="2");
    assert(c[4].empty() && c[5].empty() && c[6].empty() && c[8].empty() && c[7]=="SIN_HORA");
    assert(c[15].empty() && c[28].empty() && c[34].empty() && c[37].empty());
    auto& info=fila.snapshot.datos.infoEnergia;
    info.estado=e; info.disponible=true; info.capturaUs=1000000; info.modo=2;
    fila.hora=estimarHora(reloj,6000000,3000); fila.timestampMs=1000;
    info.estado.corrienteA=0; info.estado.potenciaW=0;
    n=formatearCSV(fila,buffer,sizeof(buffer)); c=dividir(buffer);
    assert(c[16]=="0.000" && c[17]=="0.00" && c[27]=="50.00");
    char pequeno[12]; assert(formatearCSV(fila,pequeno,sizeof(pequeno))==0 && pequeno[0]==0);
    fila.snapshot.datos.temperatura.visto1=true; fila.snapshot.datos.temperatura.valida1=true;
    fila.snapshot.datos.temperatura.temp1=30; fila.snapshot.datos.temperatura.valida2=false;
    formatearCSV(fila,buffer,sizeof(buffer)); c=dividir(buffer);
    assert(c[34]=="30.00" && c[35].empty() && c[36].empty());
    info.tensionADC=info.corrienteADC=true; info.corrienteUsada.calibrando=true; info.disponible=false;
    formatearCSV(fila,buffer,sizeof(buffer)); c=dividir(buffer);
    assert(c[16].empty() && c[49]=="CALIBRANDO");
    // Fila completa representativa, creada con el mismo formateador del firmware.
    RegistroFila completa={}; strcpy(completa.sessionId,"SIMULADO_NO_MEDICION_REAL");
    completa.snapshot.capturaUs=5000000; completa.timestampMs=2000; completa.registro=2;
    auto& ce=completa.snapshot.datos.infoEnergia;
    GestorEnergia simulacion(17,48);
    ce.estado=simulacion.actualizar(48,8.5,1); ce.capturaUs=5000000; ce.tensionUs=4900000; ce.corrienteUs=4975000;
    ce.tensionADC=ce.corrienteADC=ce.disponible=ce.activa=true; ce.modo=0; ce.tiempoIntegradoS=1;
    ce.tensionUsada.lista=true; ce.tensionUsada.voltajeBateria=48; ce.tensionUsada.voltajePinV=0.450f;
    ce.corrienteUsada.lista=true; ce.corrienteUsada.corrienteA=8.5f; ce.corrienteUsada.voltajePinV=2.7805f;
    ce.corrienteUsada.voltajeSensorV=2.7805f; ce.corrienteUsada.offsetSensorV=2.5f;
    completa.snapshot.resumen=r; completa.snapshot.resumen.coberturaUs=1000000; completa.snapshot.resumen.ventanaUs=1000000;
    completa.snapshot.resumen.corrienteMedia=8.5; completa.snapshot.resumen.corrienteMax=9; completa.snapshot.resumen.potenciaMedia=408;
    auto& ct=completa.snapshot.datos.temperatura;
    ct.temp1=30.5f; ct.temp2=31.5f; ct.promedio=31; ct.valida=ct.valida1=ct.valida2=ct.visto1=ct.visto2=true;
    ct.captura1Us=ct.captura2Us=4750000;
    auto& cg=completa.snapshot.datos.gps;
    cg.latitud=-34.6037; cg.longitud=-58.3816; cg.velocidadKmH=15.25f; cg.rumboGrados=180;
    cg.satelites=8; cg.hdop=0.9f;
    cg.ubicacionValida=cg.velocidadValida=cg.rumboValido=cg.satelitesValidos=cg.hdopValido=true;
    cg.posicionUs=cg.velocidadUs=cg.rumboUs=cg.satelitesUs=cg.hdopUs=4500000;
    auto& cv=completa.snapshot.datos.velocidad;
    cv.kmh=15.24f; cv.rpm=165.0f; cv.pulsos=22; cv.totalPulsos=110; cv.muestra=5; cv.capturaUs=4000000;
    assert(convertirUnix({2026,10,14,17,0,37,0},ms));
    completa.hora.fuente=FuenteHora::GPS; completa.hora.unixMs=ms; completa.hora.edadMs=500;
    const size_t representativa=formatearCSV(completa,buffer,sizeof(buffer));
    assert(representativa>0 && dividir(buffer).size()==70);
    if (argc>1) {
        std::ofstream ejemplo(argv[1]); ejemplo<<cabeceraCSV();
        RegistroFila vacia={}; strcpy(vacia.sessionId,"SIMULADO_NO_MEDICION_REAL");
        vacia.snapshot.datos.infoEnergia.estado.ahRestantes=17;
        vacia.snapshot.datos.infoEnergia.estado.whRestantes=816;
        vacia.snapshot.datos.infoEnergia.estado.porcentajeBateria=100;
        vacia.snapshot.capturaUs=1000000;
        formatearCSV(vacia,buffer,sizeof(buffer)); ejemplo<<buffer;
        vacia.registro=1; vacia.timestampMs=1000; vacia.snapshot.capturaUs=2000000;
        vacia.snapshot.datos.infoEnergia.capturaUs=2000000;
        vacia.snapshot.datos.infoEnergia.tensionADC=vacia.snapshot.datos.infoEnergia.corrienteADC=true;
        vacia.snapshot.datos.infoEnergia.corrienteUsada.calibrando=true;
        formatearCSV(vacia,buffer,sizeof(buffer)); ejemplo<<buffer;
        formatearCSV(completa,buffer,sizeof(buffer)); ejemplo<<buffer;
        completa.registro=3; completa.timestampMs=3000; completa.snapshot.capturaUs+=1000000;
        completa.hora.unixMs+=1000; completa.hora.edadMs+=1000;
        ce.activa=false;
        ce.estado.corrienteA=ce.estado.potenciaW=0; ce.corrienteUsada.corrienteA=0;
        ct.valida=ct.valida2=false; ct.temp2=-127;
        formatearCSV(completa,buffer,sizeof(buffer)); ejemplo<<buffer;
        completa.registro=7; completa.timestampMs=7000; completa.perdidos=3; completa.snapshot.capturaUs+=4000000;
        completa.hora.unixMs+=4000; completa.hora.edadMs+=4000; completa.hora.fuente=FuenteHora::ESTIMADA;
        formatearCSV(completa,buffer,sizeof(buffer)); ejemplo<<buffer;
    }
    // Extremos de tipo para detectar longitud: no son lecturas físicamente admisibles.
    memset(completa.sessionId,'S',55); completa.sessionId[55]=0;
    completa.timestampMs=completa.registro=completa.perdidos=completa.omitidosMonitor=UINT64_MAX;
    completa.snapshot.capturaUs=UINT64_MAX;
    ce.capturaUs=ce.tensionUs=ce.corrienteUs=ct.captura1Us=ct.captura2Us=1;
    cg.posicionUs=cg.velocidadUs=cg.rumboUs=cg.hdopUs=cg.satelitesUs=cv.capturaUs=1;
    ct.valida=ct.valida1=ct.valida2=true;
    ct.temp1=ct.temp2=ct.promedio=FLT_MAX;
    ce.estado={FLT_MAX,-FLT_MAX,FLT_MAX,FLT_MAX,FLT_MAX,FLT_MAX,FLT_MAX,FLT_MAX};
    ce.tensionUsada.voltajePinV=ce.corrienteUsada.voltajePinV=ce.corrienteUsada.voltajeSensorV=ce.corrienteUsada.offsetSensorV=FLT_MAX;
    ce.tiempoIntegradoS=FLT_MAX; ce.segmento=UINT32_MAX;
    cv.rpm=cv.kmh=cg.velocidadKmH=cg.rumboGrados=cg.hdop=FLT_MAX;
    cv.pulsos=cv.muestra=cv.perdidas=cg.satelites=UINT32_MAX; cv.totalPulsos=UINT64_MAX;
    completa.snapshot.resumen.corrienteMedia=completa.snapshot.resumen.corrienteMax=completa.snapshot.resumen.potenciaMedia=FLT_MAX;
    completa.snapshot.resumen.ventanaUs=completa.snapshot.resumen.coberturaUs=UINT64_MAX;
    completa.hora.edadMs=UINT64_MAX;
    const size_t extrema=formatearCSV(completa,buffer,sizeof(buffer));
    assert(extrema && dividir(buffer).size()==70 && extrema<sizeof(buffer));
    if (argc>2) { std::ofstream extremo(argv[2]); extremo<<cabeceraCSV()<<buffer; }
    // Dos doubles fuera del rango físico: rechazo completo si exceden el buffer.
    cg.latitud=cg.longitud=DBL_MAX;
    assert(formatearCSV(completa,buffer,sizeof(buffer))==0 && buffer[0]==0);
    std::cout<<"OK: UTC/Argentina, perdida/reajuste de hora, medias ponderadas/cobertura, archivos unicos, rename, parcial, CSV 70 columnas, ceros, temperaturas y regresion energetica.\n";
    std::cout<<"sizeof(RegistroFila) host="<<sizeof(RegistroFila)<<" encabezado="<<strlen(cabeceraCSV())<<" fila_representativa="<<representativa<<" fila_extrema="<<extrema<<"\n";
}
