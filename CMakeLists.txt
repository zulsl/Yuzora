cmake_minimum_required(VERSION 3.18)

project(Yuzora VERSION 1.0.1 LANGUAGES CXX)


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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/base
    ${CMAKE_CURRENT_SOURCE_DIR}/src/crossplatform
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/dao
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/entity
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/list
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/list/preset
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/migration
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/network
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/service/app
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/service/character
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/service/chat
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/service/chat/completion
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/service/preset
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/service/prompt
    ${CMAKE_CURRENT_SOURCE_DIR}/src/model/service/worldinfo
    ${CMAKE_CURRENT_SOURCE_DIR}/src/viewmodel
)

set_source_files_properties(qml/theme/Theme.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)
set_source_files_properties(qml/layouts/Ui.qml PROPERTIES QT_QML_SINGLETON_TYPE TRUE)

qt_add_qml_module(${PROJECT_NAME}
    URI ${PROJECT_NAME}
    VERSION 1.0
    RESOURCES res.qrc

    SOURCES
        src/base/AppException.cpp
        src/base/AppException.h
        src/base/ExceptionHandler.cpp
        src/base/ExceptionHandler.h
        src/base/GlobalConstants.cpp
        src/base/GlobalConstants.h
        src/base/MessageTextRenderer.cpp
        src/base/MessageTextRenderer.h
        src/base/ScopeTimer.cpp
        src/base/ScopeTimer.h
        src/base/Utils.cpp
        src/base/Utils.h

        src/crossplatform/DeviceHelper.cpp
        src/crossplatform/DeviceHelper.h

        src/model/ApplicationBootstrap.cpp
        src/model/ApplicationBootstrap.h
        src/model/DatabaseManager.cpp
        src/model/DatabaseManager.h

        src/model/dao/AbstractDao.cpp
        src/model/dao/AbstractDao.h
        src/model/dao/ApiConfigDao.cpp
        src/model/dao/ApiConfigDao.h
        src/model/dao/BaseDao.h
        src/model/dao/CharacterDao.cpp
        src/model/dao/CharacterDao.h
        src/model/dao/ChatMessageDao.cpp
        src/model/dao/ChatMessageDao.h
        src/model/dao/ChatSessionDao.cpp
        src/model/dao/ChatSessionDao.h
        src/model/dao/CompletionPresetDao.cpp
        src/model/dao/CompletionPresetDao.h
        src/model/dao/ConnectionProviderRegistry.cpp
        src/model/dao/ConnectionProviderRegistry.h
        src/model/dao/IConnectionProvider.h
        src/model/dao/PromptDao.cpp
        src/model/dao/PromptDao.h
        src/model/dao/SettingsDao.cpp
        src/model/dao/SettingsDao.h
        src/model/dao/TransactionScope.h
        src/model/dao/VectorMemoryDao.cpp
        src/model/dao/VectorMemoryDao.h
        src/model/dao/WorldInfoBookDao.cpp
        src/model/dao/WorldInfoBookDao.h
        src/model/dao/WorldInfoEntryDao.cpp
        src/model/dao/WorldInfoEntryDao.h

        src/model/entity/ApiConfigEntity.h
        src/model/entity/CharacterEntity.h
        src/model/entity/ChatMessageEntity.h
        src/model/entity/ChatSessionEntity.h
        src/model/entity/CompletionPresetEntity.h
        src/model/entity/PromptEntity.h
        src/model/entity/SettingsEntity.h
        src/model/entity/VectorMemoryEntity.h
        src/model/entity/WorldInfoBookEntity.h
        src/model/entity/WorldInfoEntryEntity.h

        src/model/list/ApiConfigListModel.cpp
        src/model/list/ApiConfigListModel.h
        src/model/list/CharacterItemViewModel.cpp
        src/model/list/CharacterItemViewModel.h
        src/model/list/CharacterListModel.cpp
        src/model/list/CharacterListModel.h
        src/model/list/ChatMessageItemModel.cpp
        src/model/list/ChatMessageItemModel.h
        src/model/list/ChatMessageListModel.cpp
        src/model/list/ChatMessageListModel.h
        src/model/list/ChatSessionListModel.cpp
        src/model/list/ChatSessionListModel.h
        src/model/list/WorldInfoBookItemModel.cpp
        src/model/list/WorldInfoBookItemModel.h
        src/model/list/WorldInfoBookListModel.cpp
        src/model/list/WorldInfoBookListModel.h
        src/model/list/WorldInfoEntryListModel.cpp
        src/model/list/WorldInfoEntryListModel.h

        src/model/list/preset/PresetListModel.cpp
        src/model/list/preset/PresetListModel.h
        src/model/list/preset/PresetPromptListModel.cpp
        src/model/list/preset/PresetPromptListModel.h

        src/model/migration/MigrationRunner.cpp
        src/model/migration/MigrationRunner.h
        src/model/migration/SchemaMigration.h
        src/model/migration/SchemaMigrations.cpp
        src/model/migration/SchemaMigrations.h

        src/model/network/BaseApiClient.cpp
        src/model/network/BaseApiClient.h
        src/model/network/ChatMessage.cpp
        src/model/network/ChatMessage.h
        src/model/network/ClaudeApiClient.cpp
        src/model/network/ClaudeApiClient.h
        src/model/network/DeepSeekApiClient.cpp
        src/model/network/DeepSeekApiClient.h
        src/model/network/DoubaoApiClient.cpp
        src/model/network/DoubaoApiClient.h
        src/model/network/GeminiApiClient.cpp
        src/model/network/GeminiApiClient.h
        src/model/network/ModelFactory.cpp
        src/model/network/ModelFactory.h
        src/model/network/NetworkError.cpp
        src/model/network/NetworkError.h
        src/model/network/OpenAICompatibleApiClient.cpp
        src/model/network/OpenAICompatibleApiClient.h

        src/model/service/app/ApiConfigService.cpp
        src/model/service/app/ApiConfigService.h
        src/model/service/app/SettingsService.cpp
        src/model/service/app/SettingsService.h
        src/model/service/app/StorageService.cpp
        src/model/service/app/StorageService.h

        src/model/service/character/CharacterCardParser.cpp
        src/model/service/character/CharacterCardParser.h
        src/model/service/character/CharacterService.cpp
        src/model/service/character/CharacterService.h
        src/model/service/character/GreetingService.cpp
        src/model/service/character/GreetingService.h

        src/model/service/chat/ChatService.cpp
        src/model/service/chat/ChatService.h
        src/model/service/chat/ChatServiceAssembly.cpp
        src/model/service/chat/MemoryService.cpp
        src/model/service/chat/MemoryService.h
        src/model/service/chat/SessionService.cpp
        src/model/service/chat/SessionService.h

        src/model/service/chat/completion/ApiCompletionRunner.cpp
        src/model/service/chat/completion/ApiCompletionRunner.h
        src/model/service/chat/completion/ChatCompletionRunner.h

        src/model/service/preset/CompletionPresetService.cpp
        src/model/service/preset/CompletionPresetService.h

        src/model/service/prompt/ChatCompletion.cpp
        src/model/service/prompt/ChatCompletion.h
        src/model/service/prompt/ChatMessageGroup.h
        src/model/service/prompt/ChatPromptContext.cpp
        src/model/service/prompt/ChatPromptContext.h
        src/model/service/prompt/MacroEngine.cpp
        src/model/service/prompt/MacroEngine.h
        src/model/service/prompt/PromptBuilder.cpp
        src/model/service/prompt/PromptBuilder.h

        src/model/service/worldinfo/WorldInfoMatcher.cpp
        src/model/service/worldinfo/WorldInfoMatcher.h
        src/model/service/worldinfo/WorldInfoService.cpp
        src/model/service/worldinfo/WorldInfoService.h

        src/viewmodel/AboutViewModel.cpp
        src/viewmodel/AboutViewModel.h
        src/viewmodel/ApiConfigDraft.cpp
        src/viewmodel/ApiConfigDraft.h
        src/viewmodel/ApiConfigViewModel.cpp
        src/viewmodel/ApiConfigViewModel.h
        src/viewmodel/CharacterDraft.cpp
        src/viewmodel/CharacterDraft.h
        src/viewmodel/CharacterViewModel.cpp
        src/viewmodel/CharacterViewModel.h
        src/viewmodel/ChatViewModel.cpp
        src/viewmodel/ChatViewModel.h
        src/viewmodel/CompletionPresetDraft.cpp
        src/viewmodel/CompletionPresetDraft.h
        src/viewmodel/CompletionPresetViewModel.cpp
        src/viewmodel/CompletionPresetViewModel.h
        src/viewmodel/SettingsViewModel.cpp
        src/viewmodel/SettingsViewModel.h
        src/viewmodel/WorldInfoEntryViewModel.cpp
        src/viewmodel/WorldInfoEntryViewModel.h
        src/viewmodel/WorldInfoViewModel.cpp
        src/viewmodel/WorldInfoViewModel.h

    QML_FILES
        # qml/components/chat
        qml/components/chat/MessageBubble.qml
        qml/components/chat/TypingDots.qml

        # qml/components/controls
        qml/components/controls/AppButton.qml
        qml/components/controls/AppCheckBox.qml
        qml/components/controls/ChatScrollBar.qml
        qml/components/controls/CircleAvatar.qml
        qml/components/controls/EmptyState.qml
        qml/components/controls/ExpandableRow.qml
        qml/components/controls/IconButton.qml
        qml/components/controls/LabeledField.qml
        qml/components/controls/LongTextField.qml
        qml/components/controls/SectionCard.qml
        qml/components/controls/SliderFloatInput.qml
        qml/components/controls/TagChip.qml

        # qml/components/dialogs
        qml/components/dialogs/ConfirmDialog.qml
        qml/components/dialogs/TextInputDialog.qml

        # qml/components/overlay
        qml/components/overlay/AppToast.qml
        qml/components/overlay/CameraMask.qml
        qml/components/overlay/MessageContextMenu.qml
        qml/components/overlay/TextContextMenu.qml

        # qml/layouts
        qml/layouts/AdaptiveSplit.qml
        qml/layouts/AppToolBar.qml
        qml/layouts/MasterDetailPage.qml
        qml/layouts/MasterDetailStack.qml
        qml/layouts/NavigationHost.qml
        qml/layouts/NavigationPanel.qml
        qml/layouts/NotificationBridge.qml
        qml/layouts/PageHost.qml
        qml/layouts/PageScaffold.qml
        qml/layouts/Ui.qml

        # qml
        qml/Main.qml

        # qml/pages/about
        qml/pages/about/AboutGroup.qml
        qml/pages/about/AboutPage.qml
        qml/pages/about/AboutRow.qml
        qml/pages/about/DonationDialog.qml
        qml/pages/about/PrivacyDialog.qml

        # qml/pages/api_config
        qml/pages/api_config/ApiConfigPage.qml

        # qml/pages/character
        qml/pages/character/AlternateGreetingsList.qml
        qml/pages/character/CharacterBookDialog.qml
        qml/pages/character/CharacterDetailForm.qml
        qml/pages/character/CharacterListPanel.qml
        qml/pages/character/CharacterPage.qml

        # qml/pages/chat
        qml/pages/chat/ChatPage.qml

        # qml/pages/completion_preset
        qml/pages/completion_preset/CompletionPresetPage.qml
        qml/pages/completion_preset/PresetEditForm.qml
        qml/pages/completion_preset/PresetListPanel.qml
        qml/pages/completion_preset/PromptsListView.qml

        # qml/pages/settings
        qml/pages/settings/SettingsPage.qml

        # qml/pages/world_info
        qml/pages/world_info/BookEntryListView.qml
        qml/pages/world_info/BookListPanel.qml
        qml/pages/world_info/EntryKeywordList.qml
        qml/pages/world_info/EntryListPanel.qml
        qml/pages/world_info/EntryNumberField.qml
        qml/pages/world_info/EntrySelectiveLogicComboBox.qml
        qml/pages/world_info/InsertPositionComboBox.qml
        qml/pages/world_info/TriggerStrategyComboBox.qml
        qml/pages/world_info/WorldInfoPage.qml

        # qml/theme
        qml/theme/Metrics.qml
        qml/theme/Motion.qml
        qml/theme/PaletteDark.qml
        qml/theme/PaletteLight.qml
        qml/theme/Theme.qml
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
# GPL 分发要求随二进制附上许可证；关于页只链接，不把全文塞进 QML。
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
        DESTINATION ${CMAKE_INSTALL_BINDIR})

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
