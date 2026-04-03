QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    settings_dialog.cpp \
    Action.cpp \
    Attribute.cpp \
    AttributeRegistry.cpp \
    DiceRoller.cpp \
    Effect.cpp \
    Encounter.cpp \
    PathTrackerStruct.cpp \
    Pawn.cpp \
    editor.cpp \
    editor_addeffect.cpp \
    editor_attribute.cpp \
    editor_effectheader.cpp \
    editor_encountertemplate.cpp \
    editor_list.cpp \
    editor_player.cpp \
    editor_pawntemplate.cpp \
    editor_stages.cpp \
    main.cpp \
    mainwindow.cpp \
    pawn_editor.cpp \
    pawnrow.cpp

HEADERS += \
    settings_dialog.h \
    Action.h \
    PictureHelper.h \
    Attribute.h \
    AttributeRegistry.h \
    DiceRoller.h \
    Effect.h \
    Encounter.h \
    PathTrackerStruct.h \
    PathfinderCore.h \
    Pawn.h \
    editor.h \
    editor_addeffect.h \
    editor_attribute.h \
    editor_effectheader.h \
    editor_encountertemplate.h \
    editor_list.h \
    editor_player.h \
    editor_pawntemplate.h \
    editor_stages.h \
    mainwindow.h \
    pawn_editor.h \
    pawnrow.h

FORMS += \
    settings_dialog.ui \
    editor.ui \
    editor_addeffect.ui \
    editor_attribute.ui \
    editor_effectheader.ui \
    editor_encountertemplate.ui \
    editor_list.ui \
    editor_player.ui \
    editor_pawntemplate.ui \
    editor_stages.ui \
    mainwindow.ui \
    pawn_editor.ui \
    pawnrow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
