set(TS_FILES
    qtbase_cs.ts
    qtbase_da.ts
    qtbase_de.ts
    qtbase_es.ts
    qtbase_fi.ts
    qtbase_fr.ts
    qtbase_he.ts
    qtbase_it.ts
    qtbase_ja.ts
    qtbase_lt.ts
    qtbase_pl.ts
    qtbase_pt_BR.ts
    qtbase_pt_PT.ts
    qtbase_ru.ts
    qtbase_sv.ts
    qtbase_uk.ts
    qtbase_zh_CN.ts
    qtbase_zh_TW.ts
    rssguard_cs.ts
    rssguard_da.ts
    rssguard_de.ts
    rssguard_en_GB.ts
    rssguard_en.ts
    rssguard_es.ts
    rssguard_fi.ts
    rssguard_fr.ts
    rssguard_gl.ts
    rssguard_he.ts
    rssguard_id.ts
    rssguard_it.ts
    rssguard_ja.ts
    rssguard_lt.ts
    rssguard_nl.ts
    rssguard_pl.ts
    rssguard_pt_BR.ts
    rssguard_pt_PT.ts
    rssguard_ru.ts
    rssguard_sv.ts
    rssguard_uk.ts
    rssguard_zh_CN.ts
    rssguard_zh_TW.ts
)
set_source_files_properties(${TS_FILES} PROPERTIES OUTPUT_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}")

if(UPDATE_TRANSLATIONS)
    qt_create_translation(QM_FILES
        "${CMAKE_SOURCE_DIR}/src" ${TS_FILES}
        OPTIONS -no-obsolete
    )
else()
    qt_add_translation(QM_FILES
        ${TS_FILES}
        OPTIONS -compress
    )
endif()

add_custom_target(update_qm DEPENDS ${QM_FILES})
