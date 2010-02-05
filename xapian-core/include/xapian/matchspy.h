/** @file matchspy.h
 * @brief MatchSpy implementation.
 */
/* Copyright (C) 2007,2008,2009 Olly Betts
 * Copyright (C) 2007,2009 Lemur Consulting Ltd
 * Copyright (C) 2010 Richard Boulton
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifndef XAPIAN_INCLUDED_MATCHSPY_H
#define XAPIAN_INCLUDED_MATCHSPY_H

#include <xapian/base.h>
#include <xapian/enquire.h>
#include <xapian/termiterator.h>
#include <xapian/visibility.h>

#include <string>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace Xapian {

class Document;
class Registry;

/** Abstract base class for match spies.
 *
 *  The subclasses will generally accumulate information seen during the match,
 *  to calculate aggregate functions, or other profiles of the matching
 *  documents.
 */
class XAPIAN_VISIBILITY_DEFAULT MatchSpy {
  private:
    /// Don't allow assignment.
    void operator=(const MatchSpy &);

    /// Don't allow copying.
    MatchSpy(const MatchSpy &);

  protected:
    /// Default constructor, needed by subclass constructors.
    MatchSpy() {}

  public:
    /** Virtual destructor, because we have virtual methods. */
    virtual ~MatchSpy();

    /** Register a document with the match spy.
     *
     *  This is called by the matcher once with each document seen by the
     *  matcher during the match process.  Note that the matcher will often not
     *  see all the documents which match the query, due to optimisations which
     *  allow low-weighted documents to be skipped, and allow the match process
     *  to be terminated early.
     *
     *  @param doc The document seen by the match spy.
     *  @param wt The weight of the document.
     */
    virtual void operator()(const Xapian::Document &doc,
			    Xapian::weight wt) = 0;

    /** Clone the match spy.
     *
     *  The clone should inherit the configuration of the parent, but need not
     *  inherit the state.  ie, the clone does not need to be passed
     *  information about the results seen by the parent.
     *
     *  If you don't want to support the remote backend in your match spy, you
     *  can use the default implementation which simply throws
     *  Xapian::UnimplementedError.
     *
     *  Note that the returned object will be deallocated by Xapian after use
     *  with "delete".  It must therefore have been allocated with "new".
     */
    virtual MatchSpy * clone() const;

    /** Return the name of this match spy.
     *
     *  This name is used by the remote backend.  It is passed with the
     *  serialised parameters to the remote server so that it knows which class
     *  to create.
     *
     *  Return the full namespace-qualified name of your class here - if your
     *  class is called MyApp::FooMatchSpy, return "MyApp::FooMatchSpy" from
     *  this method.
     *
     *  If you don't want to support the remote backend in your match spy, you
     *  can use the default implementation which simply throws
     *  Xapian::UnimplementedError.
     */
    virtual std::string name() const;

    /** Return this object's parameters serialised as a single string.
     *
     *  If you don't want to support the remote backend in your match spy, you
     *  can use the default implementation which simply throws
     *  Xapian::UnimplementedError.
     */
    virtual std::string serialise() const;

    /** Unserialise parameters.
     *
     *  This method unserialises parameters serialised by the @a serialise()
     *  method and allocates and returns a new object initialised with them.
     *
     *  If you don't want to support the remote backend in your match spy, you
     *  can use the default implementation which simply throws
     *  Xapian::UnimplementedError.
     *
     *  Note that the returned object will be deallocated by Xapian after use
     *  with "delete".  It must therefore have been allocated with "new".
     */
    virtual MatchSpy * unserialise(const std::string & s,
				   const Registry & context) const;

    /** Serialise the results of this match spy.
     *
     *  If you don't want to support the remote backend in your match spy, you
     *  can use the default implementation which simply throws
     *  Xapian::UnimplementedError.
     */
    virtual std::string serialise_results() const;

    /** Unserialise some results, and merge them into this matchspy.
     *
     *  The order in which results are merged should not be significant, since
     *  this order is not specified (and will vary depending on the speed of
     *  the search in each sub-database).
     *
     *  If you don't want to support the remote backend in your match spy, you
     *  can use the default implementation which simply throws
     *  Xapian::UnimplementedError.
     */
    virtual void merge_results(const std::string & s);

    /** Return a string describing this object.
     *
     *  This default implementation returns a generic answer, to avoid forcing
     *  those deriving their own MatchSpy subclasses from having to implement
     *  this (they may not care what get_description() gives for their
     *  subclass).
     */
    virtual std::string get_description() const;
};


/** Class for counting the frequencies of values in the matching documents.
 *
 *  Warning: this API is currently experimental, and is liable to change
 *  between releases without warning.
 */
class XAPIAN_VISIBILITY_DEFAULT ValueCountMatchSpy : public MatchSpy {
  public:
    struct Internal;

#ifndef SWIG // SWIG doesn't need to know about the internal class
    struct XAPIAN_VISIBILITY_DEFAULT Internal
	    : public Xapian::Internal::RefCntBase
    {
	/// The slot to count.
	Xapian::valueno slot;

	/// Total number of documents seen by the match spy.
	Xapian::doccount total;

	/// The values seen so far, together with their frequency.
	std::map<std::string, Xapian::doccount> values;

	Internal() : slot(Xapian::BAD_VALUENO), total(0) {}
	Internal(Xapian::valueno slot_) : slot(slot_), total(0) {}
    };
#endif

  protected:
    Xapian::Internal::RefCntPtr<Internal> internal;

  public:
    /// Construct an empty ValueCountMatchSpy.
    ValueCountMatchSpy() : internal() {}

    /// Construct a MatchSpy which counts the values in a particular slot.
    ValueCountMatchSpy(Xapian::valueno slot_)
	    : internal(new Internal(slot_)) {}

    /// Return the values seen in the slot.
    const std::map<std::string, Xapian::doccount> & get_values() const {
	return internal->values;
    }

    /** Return the total number of documents tallied. */
    size_t get_total() const {
	return internal->total;
    }

    /** Get an iterator over the values seen in the slot.
     *
     *  Items will be returned in ascending alphabetical order.
     *
     *  During the iteration, the frequency of the current value can be
     *  obtained with the get_termfreq() method on the iterator.
     */
    TermIterator values_begin() const;

    /** End iterator corresponding to values_begin() */
    TermIterator values_end() const {
	return TermIterator(NULL);
    }

    /** Get an iterator over the most frequent values seen in the slot.
     *
     *  Items will be returned in descending order of frequency.  Values with
     *  the same frequency will be returned in ascending alphabetical order.
     *
     *  During the iteration, the frequency of the current value can be
     *  obtained with the get_termfreq() method on the iterator.
     *
     *  @param maxvalues The maximum number of values to return.
     */
    TermIterator top_values_begin(size_t maxvalues) const;

    /** End iterator corresponding to top_values_begin() */
    TermIterator top_values_end(size_t) const {
	return TermIterator(NULL);
    }

    /** Implementation of virtual operator().
     *
     *  This implementation tallies values for a matching document.
     */
    void operator()(const Xapian::Document &doc, Xapian::weight wt);

    virtual MatchSpy * clone() const;
    virtual std::string name() const;
    virtual std::string serialise() const;
    virtual MatchSpy * unserialise(const std::string & s,
				   const Registry & context) const;
    virtual std::string serialise_results() const;
    virtual void merge_results(const std::string & s);
    virtual std::string get_description() const;
};

}

#endif // XAPIAN_INCLUDED_MATCHSPY_H