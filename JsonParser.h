/**
* \li RFC8259:  https://datatracker.ietf.org/doc/html/rfc8259
* \li JSON org: https://www.json.org/json-en.html
* \li JPATH
* 
* Constrains: 
* \li No unicode support guaranted
* \li Escape character are still visible to user. It is due to parsing in place. User receives exact place in str.
*/

#ifndef JSON_PARSER_H_
#define JSON_PARSER_H_

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* definitions */
#define MAX_DEPTH 50
#define MAX_URI_LEN 500

/* public interface */

/**
 * \brief JsonParser type definition
 *
 * JsonParser type. Client shall create at least one object to deal with api functions. Object can be reused for different documents.
 */
typedef struct _JsonParser JsonParser;

/**
 * \brief TValueInformCallback definition.
 *
 * Defines a type of function callback to be passed to JsonParser_parse as an argument. Callbacks will be called with 4 arguments, where:
 *  * key and keyLen are jpath address of the current value. It composes a string (no null termination) in the form: begin, lenght.
 *      It points to temporary object so in case user likes to use it, it needs to copy it.
 *  * value and valueLen points to value in json. It composes a string (no null termination) in the form: begin, lenght. It points to position from 
 *		original string passed to be parsed, so as long as used to not change json string it is safe to use.
 *
 * @param key begin of the key string. See jpath. Points to temporary value.
 * @param keyLen Lenght of the key sting. 
 * @param value begin of value found in json. It can point to string (no quotes), digit, json_object, json_array 
 * @param valueLen Lenght of the key sting.
 */
typedef void (*TValueInformCallback)(const char* key, int keyLen, const char* value, int valueLen);

/**
 * 
 */
int JsonParser_parse(JsonParser* parser, const char* jsonBegin, const char* jsonEnd, TValueInformCallback valueInformCallback);

/* end of public interface */

/* private part */

const char* objectUri = "/";

typedef struct _UriPart
{
	const char* Begin;
	int len;
} UriPart;

typedef struct _UriParts
{
	UriPart parts[MAX_DEPTH];
	int nParts;
} UriParts;

int UriParts_appendObject(UriParts* uriParts);
void UriParts_drop(UriParts* uriParts);
int UriParts_appendString(UriParts* uriParts, const char* begin, int len);

struct _JsonParser
{
	const char* str_;
	const char* end_;
	UriParts uriParts_;
	TValueInformCallback inform_;
	int isInvalid = true;
};

void JsonParser_inform(JsonParser* parserInstance, const char* begin, int len);
void JsonParser_parseArray(JsonParser* parserInstance);
void JsonParser_parseNumber(JsonParser* parserInstance);
void JsonParser_parseValue(JsonParser* parserInstance);
void JsonParser_parseString(JsonParser* parserInstance);
void JsonParser_parseKeyValue(JsonParser* parserInstance);
void JsonParser_parseObject(JsonParser* parserInstance);
void JsonParser_consumeWhiteSpaces(JsonParser* parserInstance);

int isWhiteSpace(char c)
{
	return c == 0x20 || c == 0x09 || c == 0x0A || c == 0x0D;
}

int UriParts_appendObject(UriParts* uriParts)
{
	UriPart uri;
	uri.Begin = objectUri;
	uri.len = 1;
	if (uriParts->nParts >= MAX_DEPTH)
	{
		return 0;
	}
	uriParts->parts[uriParts->nParts++] = uri;
	return 1;
}

void UriParts_drop(UriParts* uriParts)
{
	uriParts->nParts--;
}

int UriParts_appendString(UriParts* uriParts, const char* begin, int len)
{
	UriPart uri;
	uri.Begin = begin;
	uri.len = len;
	if (uriParts->nParts >= MAX_DEPTH)
	{
		return 0;
	}
	uriParts->parts[uriParts->nParts++] = uri;
	return 1;
}

void JsonParser_inform(JsonParser* parserInstance, const char* begin, int len)
{
	char uri[MAX_URI_LEN];
	int n = 0;
	for (int i = 0; i < parserInstance->uriParts_.nParts; ++i)
	{
		if (n + parserInstance->uriParts_.parts[i].len > MAX_URI_LEN)
		{
			parserInstance->isInvalid = 1;
			return;
		}
		memcpy(uri + n, parserInstance->uriParts_.parts[i].Begin, parserInstance->uriParts_.parts[i].len);
		n += parserInstance->uriParts_.parts[i].len;
	}

	if (!parserInstance->isInvalid)
	{
		parserInstance->inform_(uri, n, begin, len);
	}
}

void JsonParser_parseArray(JsonParser* parserInstance)
{
	int index = 0;
	char buffer[20];

	++parserInstance->str_;

	JsonParser_consumeWhiteSpaces(parserInstance);
	if (*parserInstance->str_ == ']')
	{
		++parserInstance->str_;
		return;
	}

	for (; parserInstance->str_ < parserInstance->end_ && ! parserInstance->isInvalid;)
	{
		JsonParser_consumeWhiteSpaces(parserInstance);
		if (!UriParts_appendString(&parserInstance->uriParts_, buffer, sprintf(buffer, "[%d]", index)))
		{
			parserInstance->isInvalid = 1;
			return;
		}
		JsonParser_parseValue(parserInstance);
		UriParts_drop(&parserInstance->uriParts_);
		
		JsonParser_consumeWhiteSpaces(parserInstance);
		if (*parserInstance->str_ == ',')
		{
			++parserInstance->str_;
			++index;
			continue;
		}
		if (*parserInstance->str_ == ']')
		{
			++parserInstance->str_;
			return;
		}
		parserInstance->isInvalid = 1;
		return;
	}
	return;
}

void JsonParser_parseNumber(JsonParser* parserInstance)
{
	if (*parserInstance->str_ == '-' && (
		(parserInstance->str_ + 1 == parserInstance->end_) || !isdigit(parserInstance->str_[1]) || parserInstance->str_[1] == '0'))
	{
		parserInstance->isInvalid = 1;
		return;
	}
	++parserInstance->str_;
	for (; parserInstance->str_ < parserInstance->end_; ++parserInstance->str_)
	{
		if (! isdigit(*parserInstance->str_) )
			break;
	}
	if (parserInstance->str_ != parserInstance->end_ && *parserInstance->str_ == '.')
	{
		if (parserInstance->str_ + 1 == parserInstance->end_ || !isdigit(parserInstance->str_[1]))
		{
			parserInstance->isInvalid = 1;
			return;
		}
		++parserInstance->str_;
		for (; parserInstance->str_ < parserInstance->end_; ++parserInstance->str_)
		{
			if (! isdigit(*parserInstance->str_))
				break;
		}
	}
	if (parserInstance->str_ != parserInstance->end_ &&
		( *parserInstance->str_ == 'E' || *parserInstance->str_ == 'e'))
	{
		if (parserInstance->str_ + 1 == parserInstance->end_)
		{
			parserInstance->isInvalid = 1;
			return;
		}
		++parserInstance->str_;
		if (*parserInstance->str_ == '-' || *parserInstance->str_ == '+')
		{
			if (parserInstance->str_ + 1 == parserInstance->end_)
			{
				parserInstance->isInvalid = 1;
				return;
			}
			++parserInstance->str_;
		}
		if (! isdigit(*parserInstance->str_))
		{
			parserInstance->isInvalid = 1;
			return;
		}
		for (; parserInstance->str_ < parserInstance->end_; ++parserInstance->str_)
		{
			if (! isdigit(*parserInstance->str_))
				break;
		}
	}
	return;
}

void JsonParser_consumeWhiteSpaces(JsonParser* parserInstance)
{
	for (; parserInstance->str_ < parserInstance->end_ && isWhiteSpace(*parserInstance->str_); ++parserInstance->str_)
	{
	}
}

void JsonParser_parseValue(JsonParser* parserInstance)
{
	JsonParser_consumeWhiteSpaces(parserInstance);
	if (*parserInstance->str_ == '\"')
	{
		const char* beginValue = parserInstance->str_;
		JsonParser_parseString(parserInstance);
		const char* endValue = parserInstance->str_;
		JsonParser_inform(parserInstance, beginValue, endValue - beginValue);
		JsonParser_consumeWhiteSpaces(parserInstance);
		return;
	}
	if (isdigit(*parserInstance->str_) || *parserInstance->str_ == '-')
	{
		const char* beginValue = parserInstance->str_;
		JsonParser_parseNumber(parserInstance);
		const char* endValue = parserInstance->str_;
		JsonParser_inform(parserInstance, beginValue, endValue - beginValue);
		JsonParser_consumeWhiteSpaces(parserInstance);
		return;
	}
	if (*parserInstance->str_ == '{')
	{
		const char* beginValue = parserInstance->str_;
		JsonParser_parseObject(parserInstance);
		const char* endValue = parserInstance->str_;
		JsonParser_inform(parserInstance, beginValue, endValue - beginValue);
		JsonParser_consumeWhiteSpaces(parserInstance);
		return;
	}
	if (*parserInstance->str_ == '[')
	{
		const char* beginValue = parserInstance->str_;
		JsonParser_parseArray(parserInstance);
		const char* endValue = parserInstance->str_;
		JsonParser_inform(parserInstance, beginValue, endValue - beginValue);
		JsonParser_consumeWhiteSpaces(parserInstance);
		return;
	}
	if ((parserInstance->end_ - parserInstance->str_ >= 4) && (0 == strncmp(parserInstance->str_, "true", 4)))
	{
		const char* beginValue = parserInstance->str_;
		parserInstance->str_ += 4;
		const char* endValue = parserInstance->str_;
		JsonParser_inform(parserInstance, beginValue, endValue - beginValue);
		JsonParser_consumeWhiteSpaces(parserInstance);
		return;
	}
	if ((parserInstance->end_ - parserInstance->str_ >= 4) && (0 == strncmp(parserInstance->str_, "null", 4)))
	{
		const char* beginValue = parserInstance->str_;
		parserInstance->str_ += 4;
		const char* endValue = parserInstance->str_;
		JsonParser_inform(parserInstance, beginValue, endValue - beginValue);
		JsonParser_consumeWhiteSpaces(parserInstance);
		return;
	}
	if ((parserInstance->end_ - parserInstance->str_ >= 5) && (0 == strncmp(parserInstance->str_, "false", 5)))
	{
		const char* beginValue = parserInstance->str_;
		parserInstance->str_ += 5;
		const char* endValue = parserInstance->str_;
		JsonParser_inform(parserInstance, beginValue, endValue - beginValue);
		JsonParser_consumeWhiteSpaces(parserInstance);
		return;
	}
	parserInstance->isInvalid = 1;
}

void JsonParser_parseExcapedChar(JsonParser* parserInstance)
{
	++parserInstance->str_;
	if (*parserInstance->str_ == '\"'
		|| *parserInstance->str_ == '\\'
		|| *parserInstance->str_ == '\/'
		|| *parserInstance->str_ == '\b'
		|| *parserInstance->str_ == '\f'
		|| *parserInstance->str_ == '\n'
		|| *parserInstance->str_ == '\r'
		|| *parserInstance->str_ == '\t')
	{
		++parserInstance->str_;
		return;
	}
	if (*parserInstance->str_ == 'u')
	{
		++parserInstance->str_;
		++parserInstance->str_;

		return;
	}
}

void JsonParser_parseString(JsonParser* parserInstance)
{
	++parserInstance->str_;
	for (; parserInstance->str_ < parserInstance->end_;)
	{
		if (*parserInstance->str_ == '\\')
		{
			JsonParser_parseExcapedChar(parserInstance);
			continue;
		}
		if (*parserInstance->str_ == '\"')
		{
			++parserInstance->str_;
			return;
		}
		++parserInstance->str_;
	}
}

void JsonParser_parseKeyValue(JsonParser* parserInstance)
{
	const char* beginKey = parserInstance->str_ + 1;
	JsonParser_parseString(parserInstance);
	const char* endKey = parserInstance->str_ - 1;
	if (!UriParts_appendString(&parserInstance->uriParts_, beginKey, endKey - beginKey))
	{
		parserInstance->isInvalid = 1;
		return;
	}

	for (; parserInstance->str_ < parserInstance->end_ && !parserInstance->isInvalid; ++parserInstance->str_)
	{
		if (*parserInstance->str_ == ':')
		{
			++parserInstance->str_;

			JsonParser_parseValue(parserInstance);
			UriParts_drop(&parserInstance->uriParts_);
			return;
		}
	}
	return;
}

void JsonParser_parseObject(JsonParser* parserInstance)
{
	if (!UriParts_appendObject(&parserInstance->uriParts_))
	{
		parserInstance->isInvalid = true;
		return;
	}
	
	++parserInstance->str_;
	for (; parserInstance->str_ < parserInstance->end_ && !parserInstance->isInvalid;)
	{
		if (*parserInstance->str_ == '\"')
		{
			JsonParser_parseKeyValue(parserInstance);
			continue;
		}
		if (*parserInstance->str_ == '}')
		{
			UriParts_drop(&parserInstance->uriParts_);
			++parserInstance->str_;
			return;
		}
		if (*parserInstance->str_ == ',')
		{
			++parserInstance->str_;
			continue;
		}
		if (!isWhiteSpace(*parserInstance->str_))
		{
			parserInstance->isInvalid = 1;
		}
		++parserInstance->str_;
	}
	parserInstance->isInvalid = true;
}

int JsonParser_parse(JsonParser* parser, const char* jsonBegin, const char* jsonEnd, TValueInformCallback valueInformCallback)
{
	parser->str_ = jsonBegin;
	parser->end_ = jsonEnd;
	parser->inform_ = valueInformCallback;
	parser->uriParts_.nParts = 0;
	parser->isInvalid = 0;

	JsonParser_parseValue(parser);

	return parser->uriParts_.nParts != 0
		|| parser->isInvalid
		|| parser->str_ != parser->end_;
}

/* end of private part */

#endif // JSON_PARSER_H_
