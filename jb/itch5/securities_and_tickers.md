# ITCH-5.0: Confused about Securities, Symbols, and Tickers {#securities-vs-symbols}

The ITCH-5.0 protocol suffers from a common confusion in the
securities industry.  It calls their ticker field 'Stock'.  While it
is true that most securities traded on Nasdaq are common stock
securities, many are not: Nasdaq also trades Exchange Traded Funds,
Exchange Traded Notes, and even Warrants.

And the name of a security, i.e., the string used to identify the
security in the many electronic protocols used between exchanges and
participants, is not the same as the security.  A more appropriate
name would have been 'Security Identifier', or 'Ticker', or 'Symbol'.

In software we are used to not confusing an object with the multiple
references to it.
Likewise, we should get used to not confuse stock, which is a
security, that is, a contract granting specific rights to its owner;
vs. the name of the thing, such as a ticker: a short string used to
identify a security in some contexts.

Also, software engineers in the field should be aware that the same
security may be identified by different strings in different contexts,
e.g. many exchanges used different tickers for the same security, yes,
even in the US markets.
This is particularly obvious if you start
looking at securities with suffixes, such as preferred stock.
In addition, the same tickers refers to
different securities as time goes by, for example, after some
corporate actions.

And outside the US markets it is common to use fairly
long strings (ISINs) or just numbers (in Japan) to identify
securities in computer systems.
And that the string used to identify securities in a
market feed may not be the same string used to identify them when
placing orders, or clearing.

In short, it behooves sofware engineers in the field to keep these
things straight in their designs, and in their heads too I might add.

### References

http://en.wikipedia.org/wiki/Haddocks%27_Eyes
