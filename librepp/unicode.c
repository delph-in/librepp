#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<assert.h>
#include	<wchar.h>
#include	<ctype.h>

#include	"unicode.h"

char	*tombs(wchar_t	*w)
{
	char	*str = malloc(4*(wcslen(w)+1));
	if(-1 == wcstombs(str, w, 1+4*wcslen(w)))
	{
		perror("multibyte encoding failed");
		fprintf(stderr, "on wcs `%S'\n", w);
		exit(-1);
	}
	return str;
}

int	check_mbs(char	*mbs)
{
	int		len = strlen(mbs);
	wchar_t	*wide = malloc(sizeof(wchar_t)*(len+1));
	if(-1 == mbstowcs(wide, mbs, len+1))
	{
		free(wide);
		return -1;
	}
	else
	{
		free(wide);
		return 0;
	}
}

wchar_t	*towide(char	*mbs)
{
	int		len = strlen(mbs);
	wchar_t	*wide = malloc(sizeof(wchar_t)*(len+1));
	if(-1 == mbstowcs(wide, mbs, len+1))
	{
		perror("multibyte decoding failed");
		fprintf(stderr, "on string `%s'\n", mbs);
		exit(-1);
	}
	return wide;
}

int	wide_is_number(const wchar_t	*str)
{
	while(*str)
		if(!isdigit(*str++))return 0;
	return 1;
}

int	wstrlen(const wchar_t	*str)
{
	int	l = 0;
	while(*str++)l++;
	return l;
}

wchar_t	*wstrcpy(wchar_t	*dest, const wchar_t	*src)
{
	wchar_t	*odest = dest;
	while(*src)
		*dest++ = *src++;
	*dest = 0;
	return odest;
}

wchar_t	*wstrdup(const wchar_t	*in)
{
	int	l = wstrlen(in);
	wchar_t	*out = malloc(sizeof(wchar_t)*(l+1));
	memcpy(out, in, sizeof(wchar_t)*l);
	out[l] = 0;
	return out;
}

int	wstrcmp(const wchar_t	*a, const wchar_t	*b)
{
	while(*a && *b)
	{
		if(*a != *b)return (*a) - (*b);
		a++;
		b++;
	}
	if(*a)return 1;
	if(*b)return -1;
	return 0;
}

// Boost.Regex without ICU has no Unicode general categories: \p{L} is taken
// as [[:lower:]], \p{N} is an error, and inside [...] \p is a literal 'p'.
// Rewrite the common categories as POSIX classes, which follow the locale.
static const wchar_t	*posix_class_for_property(const wchar_t	*name, int	len)
{
	static const struct { const wchar_t *prop, *cls; } props[] = {
		{L"L", L"alpha"}, {L"Lu", L"upper"}, {L"Ll", L"lower"},
		{L"N", L"digit"}, {L"Nd", L"digit"},
		{L"P", L"punct"}, {L"Z", L"space"}, {L"Zs", L"blank"},
		{NULL, NULL}};
	int	i;
	for(i=0;props[i].prop;i++)
		if(wcslen(props[i].prop) == len && !wcsncmp(props[i].prop, name, len))
			return props[i].cls;
	return NULL;
}

static wchar_t	*wide_translate_re_properties(wchar_t	*str)
{
	// "\pL" (3 chars) becomes at most "[^[:alpha:]]" (12 chars)
	wchar_t	*out = calloc(sizeof(wchar_t), 4*wcslen(str) + 1);
	wchar_t	*p = out, *q = str;
	int	in_set = 0;
	while(*q)
	{
		if(*q == '\\' && (q[1] == 'p' || q[1] == 'P'))
		{
			const wchar_t	*name = q+2, *cls = NULL;
			int	len = 0, skip = 0;
			if(*name == '{')
			{
				name++;
				while(name[len] && name[len] != '}')len++;
				if(name[len] == '}')skip = 2 + 1 + len + 1;
			}
			else if(*name)
			{
				len = 1;
				skip = 3;
			}
			if(skip)cls = posix_class_for_property(name, len);
			if(cls)
			{
				int	neg = (q[1] == 'P');
				if(in_set)p += swprintf(p, 32, L"[:%ls%ls:]", neg?L"^":L"", cls);
				else p += swprintf(p, 32, L"[%ls[:%ls:]]", neg?L"^":L"", cls);
				q += skip;
				continue;
			}
		}
		if(*q == '\\' && q[1] == 'Q')
		{
			// quoted text is copied as-is
			wchar_t	*e = q+2;
			while(*e && !(e[0] == '\\' && e[1] == 'E'))e++;
			if(*e)e += 2;
			while(q<e)*p++ = *q++;
			continue;
		}
		if(*q == '\\' && q[1])
		{
			*p++ = *q++;
			*p++ = *q++;
			continue;
		}
		if(!in_set && *q == '(' && q[1] == '?' && q[2] == '#')
		{
			// (?# comment ) is copied as-is
			wchar_t	*e = wcschr(q, ')');
			e = e ? e+1 : q + wcslen(q);
			while(q<e)*p++ = *q++;
			continue;
		}
		if(!in_set && *q == '[')
		{
			in_set = 1;
			*p++ = *q++;
			// a ']' right after '[' or '[^' is a literal
			if(*q == '^')*p++ = *q++;
			if(*q == ']')*p++ = *q++;
			continue;
		}
		if(in_set && *q == '[' && (q[1] == ':' || q[1] == '.' || q[1] == '='))
		{
			// copy [:alpha:], [.a.] and [=a=] whole; they contain a ']' of their own
			wchar_t	term[3] = {q[1], ']', 0};
			wchar_t	*e = wcsstr(q+2, term);
			if(e)
			{
				e += 2;
				while(q<e)*p++ = *q++;
				continue;
			}
		}
		if(in_set && *q == ']')in_set = 0;
		*p++ = *q++;
	}
	*p = 0;
	return out;
}

wchar_t	*wide_escape_re_braces(wchar_t	*str)
{
	int	nbr = 0, i;
	for(i=0;str[i];i++)
		if(str[i] == '{' || str[i] == '}')nbr++;
	// allocate space to escape every brace
	wchar_t	*out = calloc(sizeof(wchar_t), wcslen(str) + nbr + 1);

	wchar_t	*p = out, *q = str;
	int	in_pat = 0;
	wchar_t	prev_esc = 0;	// the escaped character if the previous token was an escape like \p
	while(*q)
	{
		if(*q == '\\' && q[1])
		{
			// escapes are copied as-is; \Q...\E is quoted text, copied as-is too
			if(q[1] == 'Q')
			{
				wchar_t	*e = q+2;
				while(*e && !(e[0] == '\\' && e[1] == 'E'))e++;
				if(*e)e += 2;
				while(q<e)*p++ = *q++;
				prev_esc = 0;
				continue;
			}
			prev_esc = q[1];
			*p++ = *q++;
			*p++ = *q++;
			continue;
		}
		if(*q == '{' && !in_pat)
		{
			int	is_range = 1, is_id;
			wchar_t	*s = q+1;
			while(*s && *s != '}')
			{
				if(!(*s >= '0' && *s <= '9') && *s!=',')
					is_range = 0;
				s++;
			}
			if(s == q+1 || !*s)is_range = 0;
			// \x{...}, \p{...} and \P{...} take a braced argument
			is_id = (prev_esc == 'x' || prev_esc == 'p' || prev_esc == 'P');
			if(is_range || is_id)
				in_pat = 1;
			else *p++ = '\\';
		}
		else if(*q == '}')
		{
			if(in_pat)in_pat = 0;
			else *p++ = '\\';
		}
		prev_esc = 0;
		*p++ = *q++;
	}
	*p = 0;
	return out;
}

wchar_t	*wide_unescape_re(wchar_t	*str)
{
	wchar_t	*out = wstrdup(str), *p = out, *q = str;

	while(*q)
	{
		if(*q=='\\' && q[1])
		{
			q+=2;
			switch(q[-1])
			{
				case	't':	*p++ = '\t';	break;
				case	'n':	*p++ = '\n';	break;
				case	'r':	*p++ = '\r';	break;
				case	'0':	*p++ = '\0';	break;
				default:
					// preserve escapes we don't understand
					*p++ = '\\';
					*p++ = q[-1];
					break;
			}
		}
		else *p++ = *q++;
	}
	*p = 0;
	wchar_t	*translated = wide_translate_re_properties(out);
	free(out);
	wchar_t	*escaped = wide_escape_re_braces(translated);
	free(translated);
	return escaped;
}
