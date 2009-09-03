// Copyright (c) 2009 - Decho Corp.

#include "mordor/common/pch.h"

#include "config.h"

#include <algorithm>

#include "mordor/common/string.h"

#ifndef WINDOWS
extern char **environ;
#endif

static void delete_nothing(ConfigVarBase *) {}

void
Config::loadFromEnvironment()
{
#ifdef WINDOWS
    wchar_t *enviro = GetEnvironmentStringsW();
    if (!enviro)
        return;
    boost::shared_ptr<wchar_t> environScope(enviro, &FreeEnvironmentStringsW);
    for (const wchar_t *env = enviro; *env; env += wcslen(env) + 1) {
        const wchar_t *equals = wcschr(env, '=');
        if (!equals)
            continue;
        if (equals == env)
            continue;
        std::string key(toUtf8(env, equals - env));
        std::string value(toUtf8(equals + 1));
#else
    if (!environ)
        return;
    for (const char *env = *environ; *env; ++env) {
        const char *equals = strchr(env, '=');
        if (!equals)
            continue;
        std::string key(env, equals - env - 1);
        std::string value(equals + 1);
#endif
        std::transform(key.begin(), key.end(), key.begin(), tolower);
        replace(key, '_', '.');
        if (key.find_first_not_of("abcdefghijklmnopqrstuvwxyz.") != std::string::npos)
            continue;
        ConfigVarBase var(key);
        ConfigVarBase::ptr ptr(&var, &delete_nothing);
        std::set<ConfigVarBase::ptr, ConfigVarBase::Comparator>::iterator it = vars().find(ptr);
        if (it != vars().end()) {
            (*it)->fromString(value);
        }
    }
}

bool
ConfigVarBase::Comparator::operator()(const ConfigVarBase::ptr &lhs,
                                      const ConfigVarBase::ptr &rhs) const
{
    ASSERT(lhs);
    ASSERT(rhs);
    return lhs->m_name < rhs->m_name;
}
