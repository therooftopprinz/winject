/*
 * Copyright (C) 2023 Prinz Rainer Buyo <mynameisrainer@gmail.com>
 *
 * MIT License 
 * 
 */

#ifndef __BFC_COMMANDMANAGER_HPP__
#define __BFC_COMMANDMANAGER_HPP__

#include <map>
#include <regex>
#include <optional>
#include <iostream>
#include <string_view>
#include <functional>

namespace bfc
{

class ArgsMap : public std::map<std::string, std::string>
{
public:
    ArgsMap() = default;
    template<typename T>
    std::optional<T> argAs(const std::string_view& pKey) const
    {
        auto findit = find(pKey.data());
        if (findit == end())
        {
            return std::nullopt;
        }
        T rv;
        std::istringstream iss(findit->second);
        iss >> rv;
        return rv;
    }
    std::optional<std::string> arg(const std::string_view& pKey)
    {
        auto findit = find(pKey.data());
        if (findit == end())
        {
            return std::nullopt;
        }
        return findit->second;
    }
};

class CommandManager
{
public:
    using CmdFnCb = std::function<std::string(ArgsMap&&)>;

    template <typename Cb>
    void addCommand(const std::string_view& pCmd, Cb&& pCallback)
    {
        mCmds.emplace(pCmd, std::forward<Cb>(pCallback));
    }

    void removeCommand(const std::string_view& pCmd)
    {
        mCmds.erase(pCmd.data());
    }
    std::string executeCommand(const std::string_view& pCmd)
    {
        using namespace std::string_literals;

        const std::regex cmdArgsPattern("^([\\S]+)[\\s]*(.*)[\\s]*$");
        std::string cmd;
        std::string args;
        ArgsMap argsMap;

        std::cmatch cmdArgsPatternMatch;
        if (std::regex_match(pCmd.data(), cmdArgsPatternMatch, cmdArgsPattern))
        {
            cmd = cmdArgsPatternMatch[1].str();
            if (3==cmdArgsPatternMatch.size())
            {
                args = cmdArgsPatternMatch[2].str();
            }
        }

        const std::regex argsPattern("([\\S]+)[\\s]*=[\\s]*([\\S]+)");
        std::smatch argsPatternMatch; 
        while (std::regex_search(args, argsPatternMatch, argsPattern))
        {
            if (3!=argsPatternMatch.size())
            {
                return "invalid argument:"s + argsPatternMatch[0].str();
            }
            argsMap.emplace(argsPatternMatch[1], argsPatternMatch[2]);
            args = argsPatternMatch.suffix();
        }
        auto cmdfnIt = mCmds.find(cmd);
        if (mCmds.end() == cmdfnIt)
        {
            return "command not found: \""s+cmd+"\"\n";
        }
        return cmdfnIt->second(std::move(argsMap));
    }
private:
    std::unordered_map<std::string, CmdFnCb> mCmds;
};

} // namespace bfc

#endif // __BFC_COMMANDMANAGER_HPP__