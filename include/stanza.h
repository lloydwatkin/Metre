/***

Copyright 2013-2016 Dave Cridland
Copyright 2014-2016 Surevine Ltd

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

***/

#ifndef STANZA__H
#define STANZA__H

#include "jid.h"
#include "xmppexcept.h"
#include "rapidxml.hpp"

#include <memory>

namespace Metre {
    class XMLStream;

    class Stanza {
    public:
        typedef enum {
            bad_request,
            conflict,
            feature_not_implemented,
            forbidden,
            gone,
            internal_server_error,
            item_not_found,
            jid_malformed,
            not_acceptable,
            not_allowed,
            not_authorized,
            policy_violation,
            recipient_unavailable,
            redirect,
            registration_required,
            remote_server_not_found,
            remote_server_timeout,
            resource_constraint,
            service_unavailable,
            subscription_required,
            undefined_condition,
            unexpected_request
        } Error;
    protected:
        const char *m_name;
        std::optional<Jid> m_from;
        std::optional<Jid> m_to;
        std::optional<std::string> m_type_str;
        std::string m_id;
        std::string m_lang;
        std::string m_payload_str;
        const char *m_payload = nullptr;
        size_t m_payload_l = 0;
        rapidxml::xml_node<> const *m_node;
    public:
        Stanza(const char *name, rapidxml::xml_node<> const *node);

        Stanza(const char *name);

        Stanza(const char *name, Jid const &from, Jid const &to, std::string const &type, std::string const &id);

        const char *name() {
            return m_name;
        }

        Jid const &to() const {
            return *m_to;
        }

        Jid const &from() const {
            return *m_from;
        }

        std::optional<std::string> const &type_str() const {
            return m_type_str;
        }

        std::string const &id() const {
            return m_id;
        }

        std::string const &lang() const {
            return m_lang;
        }

        rapidxml::xml_node<> const *node() const {
            return m_node;
        }

        void render(rapidxml::xml_document<> &d);

        std::unique_ptr<Stanza> create_bounce(Metre::base::stanza_exception const &e);

        std::unique_ptr<Stanza> create_bounce(Stanza::Error e);

        std::unique_ptr<Stanza> create_forward();

        void freeze(); // Make sure nothing is in volatile storage anymore.

    protected:
        void render_error(Stanza::Error e);

        void render_error(Metre::base::stanza_exception const &ex);
    };


    class Message : public Stanza {
    public:
        typedef enum {
            UNCHECKED, NORMAL, CHAT, HEADLINE, GROUPCHAT, ERROR
        } Type;
        static const char *name;
    private:
        mutable Type m_type;
    public:
        Message(rapidxml::xml_node<> const *node) : Stanza(name, node), m_type(UNCHECKED) {
        }

        Type type() const {
            if (m_type != UNCHECKED) return m_type;
            if (!type_str()) return m_type = NORMAL;
            std::string const &t = *type_str();
            switch (t[0]) {
                case 'n':
                    if (t == "normal") return m_type = NORMAL;
                    break;
                case 'c':
                    if (t == "chat") return m_type = CHAT;
                    break;
                case 'h':
                    if (t == "headline") return m_type = HEADLINE;
                    break;
                case 'g':
                    if (t == "groupchat") return m_type = GROUPCHAT;
                    break;
                case 'e':
                    if (t == "error") return m_type = ERROR;
                    break;
            }
        }
    };


    class Iq : public Stanza {
    public:
        typedef enum {
            UNCHECKED, GET, SET, RESULT, ERROR
        } Type;
        static const char *name;
    private:
        mutable Type m_type;
    public:
        Iq(rapidxml::xml_node<> const *node) : Stanza(name, node) {}

        Iq(Jid const &from, Jid const &to, Type t, std::string const &id);

        static const char *type_toString(Type t) {
            switch (t) {
                case GET:
                    return "get";
                case SET:
                    return "set";
                case RESULT:
                    return "result";
                case ERROR:
                    return "error";
                default:
                    return "error";
            }
        }
    };


    class Presence : public Stanza {
    public:
        static const char *name;

        Presence(rapidxml::xml_node<> const *node) : Stanza(name, node) {
        }
    };

    /*
        * Slightly hacky; used for handling the two dialback elements.
        * These are not stanzas, but behave so much like them syntactically it's silly not to use the code.
        */
    class DB : public Stanza {
    public:
        typedef enum {
            VALID, INVALID, ERROR
        } Type;

        DB(const char *name, Jid const &to, Jid const &from, std::string const &stream_id,
           std::optional<std::string> const &key);

        DB(const char *name, rapidxml::xml_node<> const *node) : Stanza(name, node) {
        }

        DB(const char *name, Jid const &to, Jid const &from, std::string const &stream_id, Type t);

        DB(const char *name, Jid const &to, Jid const &from, std::string const &stream_id, Stanza::Error e);

        std::string const &key() const {
            if (!m_type_str) {
                const_cast<DB *>(this)->freeze();
                return m_payload_str;
            } else {
                throw std::runtime_error("Keys not present in typed dialback element.");
            }
        }

        class Verify;

        class Result;
    };

    class DB::Verify : public DB {
    public:
        static const char *name;

        Verify(Jid const &to, Jid const &from, std::string const &stream_id, std::string const &key)
                : DB(name, to, from, stream_id, key) {
        }

        Verify(Jid const &to, Jid const &from, std::string const &stream_id, Type t) : DB(name, to, from, stream_id,
                                                                                          t) {}

        Verify(Jid const &to, Jid const &from, std::string const &stream_id, Stanza::Error t) : DB(name, to, from,
                                                                                                   stream_id,
                                                                                                   t) {}

        Verify(rapidxml::xml_node<> const *node) : DB(name, node) {
        }
    };

    class DB::Result : public DB {
    public:
        static const char *name;

        Result(Jid const &to, Jid const &from, std::string const &key)
                : DB(name, to, from, "", key) {
        }

        Result(Jid const &to, Jid const &from, Type t) : DB(name, to, from, "",
                                                            t) {}

        Result(Jid const &to, Jid const &from, Stanza::Error t) : DB(name, to, from, "",
                                                                     t) {}

        Result(rapidxml::xml_node<> const *node) : DB(name, node) {
        }
    };
}

#endif
