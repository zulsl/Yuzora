find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Sql Test)

qt_add_executable(YuzoraTests
    WorldInfoEntryListModelTest.cpp
    ../src/model/list/WorldInfoEntryListModel.cpp
    ../src/model/list/WorldInfoEntryListModel.h
    ../src/model/entity/WorldInfoEntryEntity.h
    ../src/base/GlobalConstants.cpp
    ../src/base/GlobalConstants.h
)

target_include_directories(YuzoraTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/base
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/entity
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/list
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/dao
)

target_link_libraries(YuzoraTests PRIVATE Qt6::Core Qt6::Sql Qt6::Test)

set_target_properties(YuzoraTests PROPERTIES WIN32_EXECUTABLE FALSE)

add_test(NAME YuzoraTests COMMAND YuzoraTests)
set_tests_properties(YuzoraTests PROPERTIES
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:D:/Qt/6.8.3/msvc2022_64/bin"
)

qt_add_executable(YuzoraBaseDaoTests
    BaseDaoProviderTest.cpp
    ../src/model/dao/BaseDao.h
    ../src/model/dao/IConnectionProvider.h
    ../src/model/dao/ConnectionProviderRegistry.cpp
    ../src/model/dao/ConnectionProviderRegistry.h
)

target_include_directories(YuzoraBaseDaoTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/base
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/dao
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/entity
)

target_link_libraries(YuzoraBaseDaoTests PRIVATE Qt6::Core Qt6::Gui Qt6::Qml Qt6::Sql Qt6::Test)
set_target_properties(YuzoraBaseDaoTests PROPERTIES WIN32_EXECUTABLE FALSE)

add_test(NAME YuzoraBaseDaoTests COMMAND YuzoraBaseDaoTests)
set_tests_properties(YuzoraBaseDaoTests PROPERTIES
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:D:/Qt/6.8.3/msvc2022_64/bin"
)

qt_add_executable(YuzoraSettingsServiceTests
    SettingsServiceTest.cpp
    ../src/model/service/SettingsService.cpp
    ../src/model/service/SettingsService.h
    ../src/model/dao/SettingsDao.cpp
    ../src/model/dao/SettingsDao.h
    ../src/model/dao/BaseDao.h
    ../src/model/dao/IConnectionProvider.h
    ../src/model/dao/ConnectionProviderRegistry.cpp
    ../src/model/dao/ConnectionProviderRegistry.h
    ../src/model/entity/SettingsEntity.h
    ../src/base/AppException.cpp
    ../src/base/AppException.h
    ../src/base/GlobalConstants.cpp
    ../src/base/GlobalConstants.h
)

target_include_directories(YuzoraSettingsServiceTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/base
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/dao
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/entity
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/service
)

target_link_libraries(YuzoraSettingsServiceTests PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Sql Qt6::Test)
set_target_properties(YuzoraSettingsServiceTests PROPERTIES WIN32_EXECUTABLE FALSE)

add_test(NAME YuzoraSettingsServiceTests COMMAND YuzoraSettingsServiceTests)
set_tests_properties(YuzoraSettingsServiceTests PROPERTIES
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:D:/Qt/6.8.3/msvc2022_64/bin"
)

qt_add_executable(YuzoraCharacterDaoTests
    CharacterDaoProviderTest.cpp
    ../src/model/dao/CharacterDao.cpp
    ../src/model/dao/CharacterDao.h
    ../src/model/dao/BaseDao.h
    ../src/model/dao/IConnectionProvider.h
    ../src/model/dao/ConnectionProviderRegistry.cpp
    ../src/model/dao/ConnectionProviderRegistry.h
    ../src/model/entity/CharacterEntity.h
    ../src/base/GlobalConstants.cpp
    ../src/base/GlobalConstants.h
)

target_include_directories(YuzoraCharacterDaoTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/base
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/dao
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/entity
)

target_link_libraries(YuzoraCharacterDaoTests PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Sql Qt6::Test)
set_target_properties(YuzoraCharacterDaoTests PROPERTIES WIN32_EXECUTABLE FALSE)

add_test(NAME YuzoraCharacterDaoTests COMMAND YuzoraCharacterDaoTests)
set_tests_properties(YuzoraCharacterDaoTests PROPERTIES
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:D:/Qt/6.8.3/msvc2022_64/bin"
)

qt_add_executable(YuzoraCharacterServiceTests
    CharacterServiceTest.cpp
    ../src/model/service/CharacterService.cpp
    ../src/model/service/CharacterService.h
    ../src/model/dao/CharacterDao.cpp
    ../src/model/dao/CharacterDao.h
    ../src/model/dao/BaseDao.h
    ../src/model/dao/IConnectionProvider.h
    ../src/model/dao/ConnectionProviderRegistry.cpp
    ../src/model/dao/ConnectionProviderRegistry.h
    ../src/model/entity/CharacterEntity.h
    ../src/base/AppException.h
    ../src/base/GlobalConstants.cpp
    ../src/base/GlobalConstants.h
)

target_include_directories(YuzoraCharacterServiceTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/base
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/dao
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/entity
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/service
)

target_link_libraries(YuzoraCharacterServiceTests PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Sql Qt6::Test)
set_target_properties(YuzoraCharacterServiceTests PROPERTIES WIN32_EXECUTABLE FALSE)

add_test(NAME YuzoraCharacterServiceTests COMMAND YuzoraCharacterServiceTests)
set_tests_properties(YuzoraCharacterServiceTests PROPERTIES
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:D:/Qt/6.8.3/msvc2022_64/bin"
)

qt_add_executable(YuzoraWorldInfoServiceTests
    WorldInfoServiceTest.cpp
    ../src/model/service/WorldInfoService.cpp
    ../src/model/service/WorldInfoService.h
    ../src/model/dao/WorldInfoBookDao.cpp
    ../src/model/dao/WorldInfoBookDao.h
    ../src/model/dao/WorldInfoEntryDao.cpp
    ../src/model/dao/WorldInfoEntryDao.h
    ../src/model/dao/BaseDao.h
    ../src/model/dao/IConnectionProvider.h
    ../src/model/dao/ConnectionProviderRegistry.cpp
    ../src/model/dao/ConnectionProviderRegistry.h
    ../src/model/entity/WorldInfoBookEntity.h
    ../src/model/entity/WorldInfoEntryEntity.h
    ../src/base/AppException.cpp
    ../src/base/AppException.h
    ../src/base/GlobalConstants.cpp
    ../src/base/GlobalConstants.h
)

target_include_directories(YuzoraWorldInfoServiceTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/base
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/dao
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/entity
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/service
)

target_link_libraries(YuzoraWorldInfoServiceTests PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Sql Qt6::Test)
set_target_properties(YuzoraWorldInfoServiceTests PROPERTIES WIN32_EXECUTABLE FALSE)

add_test(NAME YuzoraWorldInfoServiceTests COMMAND YuzoraWorldInfoServiceTests)
set_tests_properties(YuzoraWorldInfoServiceTests PROPERTIES
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:D:/Qt/6.8.3/msvc2022_64/bin"
)

qt_add_executable(YuzoraCompletionPresetServiceTests
    CompletionPresetServiceTest.cpp
    ../src/model/service/CompletionPresetService.cpp
    ../src/model/service/CompletionPresetService.h
    ../src/model/dao/CompletionPresetDao.cpp
    ../src/model/dao/CompletionPresetDao.h
    ../src/model/dao/PromptDao.cpp
    ../src/model/dao/PromptDao.h
    ../src/model/dao/BaseDao.h
    ../src/model/dao/IConnectionProvider.h
    ../src/model/dao/ConnectionProviderRegistry.cpp
    ../src/model/dao/ConnectionProviderRegistry.h
    ../src/model/entity/CompletionPresetEntity.h
    ../src/model/entity/PromptEntity.h
    ../src/base/AppException.cpp
    ../src/base/AppException.h
    ../src/base/GlobalConstants.cpp
    ../src/base/GlobalConstants.h
)

target_include_directories(YuzoraCompletionPresetServiceTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/base
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/dao
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/entity
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/service
)

target_link_libraries(YuzoraCompletionPresetServiceTests PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Sql Qt6::Test)
set_target_properties(YuzoraCompletionPresetServiceTests PROPERTIES WIN32_EXECUTABLE FALSE)

add_test(NAME YuzoraCompletionPresetServiceTests COMMAND YuzoraCompletionPresetServiceTests)
set_tests_properties(YuzoraCompletionPresetServiceTests PROPERTIES
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:D:/Qt/6.8.3/msvc2022_64/bin"
)

qt_add_executable(YuzoraCharacterDraftTests
    CharacterDraftTest.cpp
    ../src/viewmodel/CharacterDraft.cpp
    ../src/viewmodel/CharacterDraft.h
    ../src/model/entity/CharacterEntity.h
)

target_include_directories(YuzoraCharacterDraftTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/base
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/viewmodel
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/entity
)

target_link_libraries(YuzoraCharacterDraftTests PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Test)
set_target_properties(YuzoraCharacterDraftTests PROPERTIES WIN32_EXECUTABLE FALSE)

add_test(NAME YuzoraCharacterDraftTests COMMAND YuzoraCharacterDraftTests)
set_tests_properties(YuzoraCharacterDraftTests PROPERTIES
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:D:/Qt/6.8.3/msvc2022_64/bin"
)

qt_add_executable(YuzoraCompletionPresetDraftTests
    CompletionPresetDraftTest.cpp
    ../src/viewmodel/CompletionPresetDraft.cpp
    ../src/viewmodel/CompletionPresetDraft.h
    ../src/model/entity/CompletionPresetEntity.h
    ../src/model/entity/PromptEntity.h
    ../src/base/GlobalConstants.cpp
    ../src/base/GlobalConstants.h
)

target_include_directories(YuzoraCompletionPresetDraftTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/base
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/viewmodel
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/entity
)

target_link_libraries(YuzoraCompletionPresetDraftTests PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Test)
set_target_properties(YuzoraCompletionPresetDraftTests PROPERTIES WIN32_EXECUTABLE FALSE)

add_test(NAME YuzoraCompletionPresetDraftTests COMMAND YuzoraCompletionPresetDraftTests)
set_tests_properties(YuzoraCompletionPresetDraftTests PROPERTIES
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:D:/Qt/6.8.3/msvc2022_64/bin"
)

qt_add_executable(YuzoraChatServiceTests
    ChatServiceTest.cpp
    ../src/model/service/ChatService.cpp
    ../src/model/service/ChatService.h
    ../src/model/service/CharacterService.cpp
    ../src/model/service/CharacterService.h
    ../src/model/service/SettingsService.cpp
    ../src/model/service/SettingsService.h
    ../src/model/dao/ChatSessionDao.cpp
    ../src/model/dao/ChatSessionDao.h
    ../src/model/dao/ChatMessageDao.cpp
    ../src/model/dao/ChatMessageDao.h
    ../src/model/dao/CharacterDao.cpp
    ../src/model/dao/CharacterDao.h
    ../src/model/dao/SettingsDao.cpp
    ../src/model/dao/SettingsDao.h
    ../src/model/dao/BaseDao.h
    ../src/model/dao/IConnectionProvider.h
    ../src/model/dao/ConnectionProviderRegistry.cpp
    ../src/model/dao/ConnectionProviderRegistry.h
    ../src/model/service/completion/ChatCompletionRunner.h
    ../src/model/service/SessionService.cpp
    ../src/model/service/SessionService.h
    ../src/model/service/greeting/GreetingService.cpp
    ../src/model/service/greeting/GreetingService.h
    ../src/model/service/TransactionScope.h
    ../src/model/entity/CharacterEntity.h
    ../src/model/entity/ChatMessageEntity.h
    ../src/model/entity/ChatSessionEntity.h
    ../src/model/entity/SettingsEntity.h
    ../src/model/entity/ApiConfigEntity.h
    ../src/model/entity/CompletionPresetEntity.h
    ../src/model/entity/PromptEntity.h
    ../src/base/AppException.h
    ../src/base/GlobalConstants.cpp
    ../src/base/GlobalConstants.h
)

target_include_directories(YuzoraChatServiceTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/base
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/dao
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/entity
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/service
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/service/prompt
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/network
)

target_link_libraries(YuzoraChatServiceTests PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Sql Qt6::Test)
set_target_properties(YuzoraChatServiceTests PROPERTIES WIN32_EXECUTABLE FALSE)

add_test(NAME YuzoraChatServiceTests COMMAND YuzoraChatServiceTests)
set_tests_properties(YuzoraChatServiceTests PROPERTIES
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:D:/Qt/6.8.3/msvc2022_64/bin"
)

qt_add_executable(YuzoraApiConfigViewModelTests
    ApiConfigViewModelTest.cpp
    ../src/viewmodel/ApiConfigViewModel.cpp
    ../src/viewmodel/ApiConfigViewModel.h
    ../src/viewmodel/ApiConfigDraft.cpp
    ../src/viewmodel/ApiConfigDraft.h
    ../src/model/service/ApiConfigService.cpp
    ../src/model/service/ApiConfigService.h
    ../src/model/dao/ApiConfigDao.cpp
    ../src/model/dao/ApiConfigDao.h
    ../src/model/list/ApiConfigListModel.cpp
    ../src/model/list/ApiConfigListModel.h
    ../src/model/dao/BaseDao.h
    ../src/model/dao/IConnectionProvider.h
    ../src/model/dao/ConnectionProviderRegistry.cpp
    ../src/model/dao/ConnectionProviderRegistry.h
    ../src/model/network/BaseApiClient.cpp
    ../src/model/network/BaseApiClient.h
    ../src/model/network/NetworkError.cpp
    ../src/model/network/NetworkError.h
    ../src/model/network/ChatMessage.cpp
    ../src/model/network/ChatMessage.h
    ../src/model/entity/ApiConfigEntity.h
    ../src/model/entity/CompletionPresetEntity.h
    ../src/model/entity/PromptEntity.h
    ../src/base/GlobalConstants.cpp
    ../src/base/GlobalConstants.h
)

target_include_directories(YuzoraApiConfigViewModelTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/base
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/dao
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/entity
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/list
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/service
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/network
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/viewmodel
)

target_link_libraries(YuzoraApiConfigViewModelTests PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Sql Qt6::Network Qt6::Test)
set_target_properties(YuzoraApiConfigViewModelTests PROPERTIES WIN32_EXECUTABLE FALSE)

add_test(NAME YuzoraApiConfigViewModelTests COMMAND YuzoraApiConfigViewModelTests)
set_tests_properties(YuzoraApiConfigViewModelTests PROPERTIES
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:D:/Qt/6.8.3/msvc2022_64/bin"
)

qt_add_executable(YuzoraPromptBuilderTests
    PromptBuilderTest.cpp
    ../src/model/service/prompt/PromptBuilder.cpp
    ../src/model/service/prompt/PromptBuilder.h
    ../src/model/service/prompt/MacroEngine.cpp
    ../src/model/service/prompt/MacroEngine.h
    ../src/model/service/prompt/ChatCompletion.h
    ../src/model/service/prompt/ChatMessageGroup.h
    ../src/model/network/ChatMessage.cpp
    ../src/model/network/ChatMessage.h
    ../src/model/service/WorldInfoMatcher.h
    ../src/model/entity/CharacterEntity.h
    ../src/model/entity/ChatMessageEntity.h
    ../src/model/entity/CompletionPresetEntity.h
    ../src/model/entity/WorldInfoEntryEntity.h
    ../src/model/entity/PromptEntity.h
    ../src/base/AppException.cpp
    ../src/base/AppException.h
    ../src/base/GlobalConstants.cpp
    ../src/base/GlobalConstants.h
)

target_include_directories(YuzoraPromptBuilderTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/base
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/network
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/entity
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/service
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/service/prompt
)

target_link_libraries(YuzoraPromptBuilderTests PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Test)
set_target_properties(YuzoraPromptBuilderTests PROPERTIES WIN32_EXECUTABLE FALSE)

add_test(NAME YuzoraPromptBuilderTests COMMAND YuzoraPromptBuilderTests)
set_tests_properties(YuzoraPromptBuilderTests PROPERTIES
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:D:/Qt/6.8.3/msvc2022_64/bin"
)

qt_add_executable(YuzoraWorldInfoMatcherTests
    WorldInfoMatcherTest.cpp
    ../src/model/service/WorldInfoMatcher.cpp
    ../src/model/service/WorldInfoMatcher.h
    ../src/model/entity/WorldInfoEntryEntity.h
    ../src/model/entity/ChatMessageEntity.h
    ../src/model/entity/CharacterEntity.h
    ../src/model/network/ChatMessage.h
    ../src/base/GlobalConstants.cpp
    ../src/base/GlobalConstants.h
)

target_include_directories(YuzoraWorldInfoMatcherTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/base
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/network
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/entity
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/service
)

target_link_libraries(YuzoraWorldInfoMatcherTests PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Test)
set_target_properties(YuzoraWorldInfoMatcherTests PROPERTIES WIN32_EXECUTABLE FALSE)

add_test(NAME YuzoraWorldInfoMatcherTests COMMAND YuzoraWorldInfoMatcherTests)
set_tests_properties(YuzoraWorldInfoMatcherTests PROPERTIES
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:D:/Qt/6.8.3/msvc2022_64/bin"
)

# 宏引擎：语料取自 SillyTavern 自己的 e2e 用例（tests/frontend/MacroEngine.e2e.js）
qt_add_executable(YuzoraMacroEngineTests
    MacroEngineTest.cpp
    ../src/model/service/prompt/MacroEngine.cpp
    ../src/model/service/prompt/MacroEngine.h
)

target_include_directories(YuzoraMacroEngineTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/service/prompt
)

target_compile_definitions(YuzoraMacroEngineTests PRIVATE
    TEST_FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures"
)

target_link_libraries(YuzoraMacroEngineTests PRIVATE Qt6::Core Qt6::Test)
set_target_properties(YuzoraMacroEngineTests PROPERTIES WIN32_EXECUTABLE FALSE)

add_test(NAME YuzoraMacroEngineTests COMMAND YuzoraMacroEngineTests)
set_tests_properties(YuzoraMacroEngineTests PROPERTIES
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:D:/Qt/6.8.3/msvc2022_64/bin"
)

# 网络错误分类：纯函数，不依赖真实网络
qt_add_executable(YuzoraNetworkErrorTests
    NetworkErrorTest.cpp
    ../src/model/network/NetworkError.cpp
    ../src/model/network/NetworkError.h
)

target_include_directories(YuzoraNetworkErrorTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/network
)

target_link_libraries(YuzoraNetworkErrorTests PRIVATE Qt6::Core Qt6::Network Qt6::Test)
set_target_properties(YuzoraNetworkErrorTests PROPERTIES WIN32_EXECUTABLE FALSE)

add_test(NAME YuzoraNetworkErrorTests COMMAND YuzoraNetworkErrorTests)
set_tests_properties(YuzoraNetworkErrorTests PROPERTIES
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:D:/Qt/6.8.3/msvc2022_64/bin"
)

qt_add_executable(YuzoraPresetPromptListModelTests
    PresetPromptListModelTest.cpp
    ../src/model/list/preset/PresetPromptListModel.cpp
    ../src/model/list/preset/PresetPromptListModel.h
    ../src/model/entity/PromptEntity.h
    ../src/base/GlobalConstants.cpp
    ../src/base/GlobalConstants.h
)

target_include_directories(YuzoraPresetPromptListModelTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/base
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/entity
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/model/list/preset
)

target_link_libraries(YuzoraPresetPromptListModelTests PRIVATE
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Test)
set_target_properties(YuzoraPresetPromptListModelTests PROPERTIES WIN32_EXECUTABLE FALSE)

add_test(NAME YuzoraPresetPromptListModelTests COMMAND YuzoraPresetPromptListModelTests)
set_tests_properties(YuzoraPresetPromptListModelTests PROPERTIES
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:D:/Qt/6.8.3/msvc2022_64/bin"
)

# 对账工具自身的回归（不依赖用户数据）：
#   prompt_diff.py --selftest 覆盖 identical/content/role/only-ours/only-st/换行 等分类。
find_package(Python3 COMPONENTS Interpreter QUIET)
if(Python3_Interpreter_FOUND)
    add_test(NAME PromptDiffToolSelftest
        COMMAND ${Python3_EXECUTABLE}
                ${CMAKE_CURRENT_SOURCE_DIR}/../tools/prompt_diff.py --selftest)
endif()
