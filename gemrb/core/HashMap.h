/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdlib.h>

#include <set>
#include <map>

#include "System/Logging.h"

template<class T>
class HashMap {
public:
	void set(const char *key, const T &value, unsigned int startValue = 0);
	void replace(const char *key, const T &value, unsigned int startValue = 0);
	const T *get(const char *key, unsigned int startValue = 0) const;
	bool has(const char *key, unsigned int startValue = 0) const;
	void remove(const char *key, unsigned int startValue = 0);
	void clear();

protected:
	typedef std::pair<unsigned int, T> Pair;
	typedef std::map<unsigned int, T> Map;

	Map map;

	unsigned int hash(const char *key, unsigned int startValue) const;
};

template<class T>
void HashMap<T>::set(const char *key, const T &value, unsigned int startValue)
{
	unsigned int h = hash(key, startValue);

	typename Map::const_iterator it = map.find(h);
	if (it != map.end()) {
		printMessage("HashMap","Hash collision or double insert on '%s': %d\n", LIGHT_RED, key, h);
		abort();
	}

	map.insert(Pair(h, value));
}

template<class T>
void HashMap<T>::replace(const char *key, const T &value, unsigned int startValue)
{
	unsigned int h = hash(key, startValue);

	map.insert(Pair(h, value));
}

template<class T>
const T *HashMap<T>::get(const char *key, unsigned int startValue) const
{
	typename Map::const_iterator it = map.find(hash(key, startValue));

	if (it == map.end())
		return 0;

	return &it->second;
}

template<class T>
bool HashMap<T>::has(const char *key, unsigned int startValue) const
{
	const T *p = get(key, startValue);

	return p != 0;
}

template<class T>
void HashMap<T>::remove(const char *key, unsigned int startValue)
{
	map.erase(hash(key, startValue));
}

template<class T>
void HashMap<T>::clear()
{
	map.clear();
}

// borrowed from CPython
template<class T>
unsigned int HashMap<T>::hash(const char *key, unsigned int startValue) const
{
	unsigned int h = startValue;
	unsigned int size = 0;
	char c;

	while ((c = *key++)) {
		h = (1000003 * h) ^ tolower(c);
		size++;
	}

	return h ^ size;
}

#endif
