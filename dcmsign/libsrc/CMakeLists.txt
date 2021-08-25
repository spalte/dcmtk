# create library from source files
DCMTK_ADD_LIBRARY(
  dcmdsig dcsighlp dcsignat siautopr sibrsapr sicert sicertvf sicreapr
  sidsa siecdsa simaccon simd5 sinullpr siprivat sipurpos siripemd sirsa
  sisha1 sisprof sisrpr sisrvpr sitstamp sitypes sisha256 sisha384
  sisha512 sitsfs)

DCMTK_TARGET_LINK_MODULES(dcmdsig ofstd dcmdata)
DCMTK_TARGET_LINK_LIBRARIES(dcmdsig ${OPENSSL_LIBS})

if (TARGET dcmdsig)
  set_property(TARGET dcmdsig PROPERTY OUTPUT_NAME "dcmdsig${DCMTK_TLS_LIBRARY_POSTFIX}")
endif()
