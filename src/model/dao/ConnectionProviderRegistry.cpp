#include "ConnectionProviderRegistry.h"

#include "IConnectionProvider.h"

namespace {
IConnectionProvider* s_provider = nullptr;
}

void ConnectionProviderRegistry::setProvider(IConnectionProvider* provider)
{
    s_provider = provider;
}

IConnectionProvider* ConnectionProviderRegistry::provider()
{
    return s_provider;
}
