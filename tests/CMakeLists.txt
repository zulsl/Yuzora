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
