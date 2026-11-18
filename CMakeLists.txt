cmake_minimum_required(VERSION 3.18)

project(Yuzora VERSION 1.0.0 LANGUAGES CXX)


set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Network Sql)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(${PROJECT_NAME}
    main.cpp
)

# 宏定义,版本号唯一权威来源
add_definitions(-DAPP_VERSION="${PROJECT_VERSION}")

# Windows：把 .ico 编进 exe（资源管理器/任务栏固定项显示的图标）。
# Qt 的 setWindowIcon 只改运行时的窗口图标，不改 exe 文件本身的图标。
# OBJECT_DEPENDS 让 .ico 变化时 rc 会重编——否则 CMake 只盯着 app.rc，换了图标 exe 还是旧的。
if(WIN32)
    target_sources(${PROJECT_NAME} PRIVATE app.rc)
    set_source_files_properties(app.rc PROPERTIES
        OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/res/app/yuzora.ico")
endif()

if(ANDROID)
    # 安卓openssl需自己导入
    include("D:/Qt/Android/SDK/android_openssl/android_openssl.cmake")
    add_android_openssl_libraries(${PROJECT_NAME})

    # 自定义清单：android:windowSoftInputMode="adjustResize"
    # 软键盘弹出时收缩窗口（只让底部让位），而不是默认的 pan（整个界面被平移）。
    set_property(TARGET ${PROJECT_NAME} PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
                 "${CMAKE_CURRENT_SOURCE_DIR}/android")
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/crossplatform
    ${CMAKE_CURRENT_SOURCE_DIR}/src/base
    ${CMAKE_CURRENT_SOURCE_DIR}/src/viewmodel
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/dao
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/entity
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/network
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/service
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/service/prompt
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/Repository
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/list
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/list/preset
)

set_source_files_properties(qml/theme/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(qml/layouts/Ui.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(${PROJECT_NAME}
    URI ${PROJECT_NAME}
    VERSION 1.0
    RESOURCES res.qrc
    SOURCES
        src/base/AppException.cpp src/base/AppException.h
        src/base/ExceptionHandler.cpp
        src/base/ExceptionHandler.h
        src/base/GlobalConstants.cpp
        src/base/GlobalConstants.h src/base/Utils.cpp
        src/base/Utils.h        src/crossplatform/DeviceHelper.cpp
        src/crossplatform/DeviceHelper.h
        src/model/DatabaseManager.cpp src/model/DatabaseManager.h
        src/model/ApplicationBootstrap.cpp src/model/ApplicationBootstrap.h
        src/viewmodel/ApiConfigViewModel.h src/viewmodel/CharacterViewModel.h src/viewmodel/CharacterDraft.h src/viewmodel/ChatViewModel.h src/viewmodel/CompletionPresetViewModel.h src/viewmodel/CompletionPresetDraft.h src/viewmodel/SettingsViewModel.h src/viewmodel/WorldInfoViewModel.h src/viewmodel/WorldInfoEntryViewModel.h
        src/model/dao/CharacterDao.h src/model/dao/ChatMessageDao.h src/model/dao/ChatSessionDao.h src/model/dao/CompletionPresetDao.h src/model/dao/SettingsDao.h src/model/dao/VectorMemoryDao.h src/model/dao/WorldInfoBookDao.h
        src/model/network/BaseApiClient.cpp src/model/network/BaseApiClient.h src/model/network/NetworkError.cpp src/model/network/NetworkError.h src/model/network/ClaudeApiClient.cpp src/model/network/ClaudeApiClient.h src/model/network/DeepSeekApiClient.cpp src/model/network/DeepSeekApiClient.h src/model/network/DoubaoApiClient.cpp src/model/network/DoubaoApiClient.h src/model/network/GeminiApiClient.cpp src/model/network/GeminiApiClient.h src/model/network/ModelFactory.cpp src/model/network/ModelFactory.h
        src/model/service/CharacterCardParser.h src/model/service/ChatService.h src/model/service/MemoryService.h src/model/service/WorldInfoMatcher.h
        src/model/list/CharacterListModel.h src/model/list/ChatMessageListModel.h
        src/model/dao/CharacterDao.cpp src/model/dao/ChatMessageDao.cpp src/model/dao/ChatSessionDao.cpp src/model/dao/CompletionPresetDao.cpp src/model/dao/SettingsDao.cpp src/model/dao/VectorMemoryDao.cpp src/model/dao/WorldInfoBookDao.cpp
        src/model/service/CharacterCardParser.cpp src/model/service/ChatService.cpp src/model/service/MemoryService.cpp src/model/service/WorldInfoMatcher.cpp
        src/model/service/ChatServiceAssembly.cpp src/model/service/TransactionScope.h
        src/model/service/SessionService.cpp src/model/service/SessionService.h
        src/model/service/completion/ChatCompletionRunner.h src/model/service/completion/ApiCompletionRunner.cpp src/model/service/completion/ApiCompletionRunner.h
        src/model/service/greeting/GreetingService.cpp src/model/service/greeting/GreetingService.h
        src/viewmodel/ApiConfigViewModel.cpp src/viewmodel/ApiConfigDraft.cpp src/viewmodel/ApiConfigDraft.h src/viewmodel/CharacterViewModel.cpp src/viewmodel/CharacterDraft.cpp src/viewmodel/ChatViewModel.cpp src/viewmodel/CompletionPresetViewModel.cpp src/viewmodel/CompletionPresetDraft.cpp src/viewmodel/SettingsViewModel.cpp src/viewmodel/WorldInfoViewModel.cpp src/viewmodel/WorldInfoEntryViewModel.cpp
        src/model/list/CharacterListModel.cpp src/model/list/ChatMessageListModel.cpp
        src/model/list/ChatMessageItemModel.cpp src/model/list/ChatMessageItemModel.h
        src/model/dao/ApiConfigDao.h src/model/dao/ApiConfigDao.cpp
        src/model/service/ApiConfigService.h src/model/service/ApiConfigService.cpp
        src/model/list/ApiConfigListModel.h src/model/list/ApiConfigListModel.cpp
        src/model/network/OpenAICompatibleApiClient.h src/model/network/OpenAICompatibleApiClient.cpp
        src/model/dao/AbstractDao.h src/model/dao/AbstractDao.cpp
        src/model/dao/IConnectionProvider.h
        src/model/dao/ConnectionProviderRegistry.cpp src/model/dao/ConnectionProviderRegistry.h
        src/model/dao/BaseDao.h
        src/model/service/CharacterService.h src/model/service/CharacterService.cpp
        src/model/list/CharacterItemViewModel.cpp src/model/list/CharacterItemViewModel.h
        src/model/service/CompletionPresetService.h src/model/service/CompletionPresetService.cpp
        src/model/network/ChatMessage.h src/model/network/ChatMessage.cpp
        src/model/service/SettingsService.h src/model/service/SettingsService.cpp
        src/model/service/StorageService.h src/model/service/StorageService.cpp
        src/model/service/WorldInfoService.h src/model/service/WorldInfoService.cpp
        src/model/list/WorldInfoBookListModel.h src/model/list/WorldInfoBookListModel.cpp
        src/model/list/WorldInfoEntryListModel.h src/model/list/WorldInfoEntryListModel.cpp
        src/model/list/WorldInfoBookItemModel.h src/model/list/WorldInfoBookItemModel.cpp
        src/model/list/ChatSessionListModel.h src/model/list/ChatSessionListModel.cpp
        src/model/entity/ApiConfigEntity.h src/model/entity/CharacterEntity.h src/model/entity/ChatMessageEntity.h src/model/entity/ChatSessionEntity.h src/model/entity/CompletionPresetEntity.h src/model/entity/PromptEntity.h src/model/entity/SettingsEntity.h src/model/entity/VectorMemoryEntity.h src/model/entity/WorldInfoBookEntity.h src/model/entity/WorldInfoEntryEntity.h
        src/model/service/prompt/PromptBuilder.cpp src/model/service/prompt/PromptBuilder.h
        src/model/service/prompt/MacroEngine.cpp src/model/service/prompt/MacroEngine.h
        src/model/list/preset/PresetPromptListModel.cpp src/model/list/preset/PresetPromptListModel.h
        src/model/list/preset/PresetListModel.cpp src/model/list/preset/PresetListModel.h
        src/model/dao/PromptDao.h src/model/dao/PromptDao.cpp
        src/base/ScopeTimer.h src/base/ScopeTimer.cpp
        src/model/dao/WorldInfoEntryDao.h src/model/dao/WorldInfoEntryDao.cpp
        src/model/service/prompt/ChatCompletion.cpp src/model/service/prompt/ChatCompletion.h
        src/model/service/prompt/ChatPromptContext.cpp src/model/service/prompt/ChatPromptContext.h
        src/model/service/prompt/ChatMessageGroup.h

    QML_FILES
        qml/Main.qml
        qml/pages/ApiConfigPage.qml
        qml/pages/ChatPage.qml
        qml/pages/CompletionPresetPage.qml
        qml/components/IconButton.qml
        qml/components/AppCheckBox.qml
        qml/components/TextContextMenu.qml
        qml/components/AppButton.qml
        qml/components/AppToolBar.qml
        qml/components/AppToast.qml
        qml/components/CircleAvatar.qml
        qml/components/TagChip.qml
        qml/components/EmptyState.qml
        qml/components/ConfirmDialog.qml
        qml/components/TextInputDialog.qml
        qml/components/SectionCard.qml
        qml/components/LabeledField.qml
        qml/components/LongTextField.qml
        qml/components/ChatScrollBar.qml
        qml/components/MessageBubble.qml
        qml/components/MessageContextMenu.qml
        qml/components/TypingDots.qml
        qml/components/ExpandableRow.qml
        qml/components/CameraMask.qml
        qml/pages/character_page/CharacterDetailForm.qml
        qml/pages/character_page/CharacterBookDialog.qml
        qml/pages/character_page/AlternateGreetingsList.qml
        qml/pages/character_page/CharacterListPanel.qml
        qml/pages/character_page/CharacterPage.qml
        qml/pages/SettingsPage.qml
        qml/pages/WorldInfoPage.qml
        qml/components/SliderFloatInput.qml
        qml/pages/completion_preset_page/PromptsListView.qml
        qml/pages/completion_preset_page/PresetListPanel.qml
        qml/pages/completion_preset_page/PresetEditForm.qml
        qml/pages/worldInfo_page/BookListPanel.qml
        qml/pages/worldInfo_page/EntryListPanel.qml
        qml/theme/Theme.qml
        qml/theme/PaletteLight.qml
        qml/theme/PaletteDark.qml
        qml/theme/Metrics.qml
        qml/theme/Motion.qml
        qml/layouts/Ui.qml
        qml/layouts/PageScaffold.qml
        qml/layouts/AdaptiveSplit.qml
        qml/layouts/MasterDetailStack.qml
        qml/layouts/NotificationBridge.qml
        qml/layouts/PageHost.qml
        qml/layouts/MasterDetailPage.qml
        qml/layouts/NavigationHost.qml
        qml/layouts/NavigationPanel.qml
        qml/pages/worldInfo_page/BookEntryListView.qml
        qml/pages/worldInfo_page/TriggerStrategyComboBox.qml
        qml/pages/worldInfo_page/InsertPositionComboBox.qml
        qml/pages/worldInfo_page/EntryNumberField.qml
        qml/pages/worldInfo_page/EntryKeywordList.qml
        qml/pages/worldInfo_page/EntrySelectiveLogicComboBox.qml

)



# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(${PROJECT_NAME} PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.Yuzora
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE Qt6::Core Qt6::Gui Qt6::Quick Qt6::QuickControls2
    Qt6::Network Qt6::Qml Qt6::Sql
)

include(GNUInstallDirs)
install(TARGETS Yuzora
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
