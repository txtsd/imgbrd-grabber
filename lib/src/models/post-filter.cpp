#include "post-filter.h"
#include <QDateTime>
#include <QObject>


int toDate(const QString &text)
{
	QDateTime date = QDateTime::fromString(text, "yyyy-MM-dd");
	if (date.isValid())
	{ return date.toString("yyyyMMdd").toInt(); }
	date = QDateTime::fromString(text, "MM/dd/yyyy");
	if (date.isValid())
	{ return date.toString("yyyyMMdd").toInt(); }
	return 0;
}

QString PostFilter::match(const QMap<QString, Token> &tokens, QString filter, bool invert)
{
	// Invert the filter by prepending '-'
	if (filter.startsWith('-'))
	{
		filter = filter.right(filter.length() - 1);
		invert = !invert;
	}

	// Meta-tags
	if (filter.contains(":"))
	{
		QString type = filter.section(':', 0, 0).toLower();
		filter = filter.section(':', 1).toLower();
		if (!tokens.contains(type))
		{ return QObject::tr("unknown type \"%1\" (available types: \"%2\")").arg(type, tokens.keys().join("\", \"")); }

		QVariant token = tokens[type].value();
		if (token.type() == QVariant::Int || token.type() == QVariant::DateTime)
		{
			int input = 0;
			if (token.type() == QVariant::Int)
			{ input = token.toInt(); }
			else if (token.type() == QVariant::DateTime)
			{ input = token.toDateTime().toString("yyyyMMdd").toInt(); }

			bool cond;
			if (token.type() == QVariant::DateTime)
			{
				if (filter.startsWith("..") || filter.startsWith("<="))
				{ cond = input <= toDate(filter.right(filter.size()-2)); }
				else if (filter.endsWith(".."))
				{ cond = input >= toDate(filter.left(filter.size()-2)); }
				else if (filter.startsWith(">="))
				{ cond = input >= toDate(filter.right(filter.size()-2)); }
				else if (filter.startsWith("<"))
				{ cond = input < toDate(filter.right(filter.size()-1)); }
				else if (filter.startsWith(">"))
				{ cond = input > toDate(filter.right(filter.size()-1)); }
				else if (filter.contains(".."))
				{ cond = input >= toDate(filter.left(filter.indexOf(".."))) && input <= toDate(filter.right(filter.size()-filter.indexOf("..")-2));	}
				else
				{ cond = input == toDate(filter); }
			}
			else
			{
				if (filter.startsWith("..") || filter.startsWith("<="))
				{ cond = input <= filter.right(filter.size()-2).toInt(); }
				else if (filter.endsWith(".."))
				{ cond = input >= filter.left(filter.size()-2).toInt(); }
				else if (filter.startsWith(">="))
				{ cond = input >= filter.right(filter.size()-2).toInt(); }
				else if (filter.startsWith("<"))
				{ cond = input < filter.right(filter.size()-1).toInt(); }
				else if (filter.startsWith(">"))
				{ cond = input > filter.right(filter.size()-1).toInt(); }
				else if (filter.contains(".."))
				{ cond = input >= filter.left(filter.indexOf("..")).toInt() && input <= filter.right(filter.size()-filter.indexOf("..")-2).toInt();	}
				else
				{ cond = input == filter.toInt(); }
			}

			if (!cond && !invert)
			{ return QObject::tr("image's %1 does not match").arg(type); }
			if (cond && invert)
			{ return QObject::tr("image's %1 match").arg(type); }
		}
		else
		{
			if (type == "rating")
			{
				QMap<QString, QString> assoc;
				assoc["s"] = "safe";
				assoc["q"] = "questionable";
				assoc["e"] = "explicit";

				if (assoc.contains(filter))
					filter = assoc[filter];

				bool cond = token.toString().toLower().startsWith(filter.left(1));
				if (!cond && !invert)
				{ return QObject::tr("image is not \"%1\"").arg(filter); }
				if (cond && invert)
				{ return QObject::tr("image is \"%1\"").arg(filter); }
			}
			else if (type == "source")
			{
				QRegExp rx(filter + "*", Qt::CaseInsensitive, QRegExp::Wildcard);
				bool cond = rx.exactMatch(token.toString());
				if (!cond && !invert)
				{ return QObject::tr("image's source does not starts with \"%1\"").arg(filter); }
				if (cond && invert)
				{ return QObject::tr("image's source starts with \"%1\"").arg(filter); }
			}
			else
			{
				QString input = token.toString();

				bool cond = input == filter;

				if (!cond && !invert)
				{ return QObject::tr("image's %1 does not match").arg(type); }
				if (cond && invert)
				{ return QObject::tr("image's %1 match").arg(type); }
			}
		}
	}
	else if (!filter.isEmpty())
	{
		QStringList tags = tokens["allos"].value().toStringList();

		// Check if any tag match the filter (case insensitive plain text with wildcards allowed)
		bool cond = false;
		for (const QString &tag : tags)
		{
			QRegExp reg(filter.trimmed(), Qt::CaseInsensitive, QRegExp::Wildcard);
			if (reg.exactMatch(tag))
			{
				cond = true;
				break;
			}
		}

		if (!cond && !invert)
		{ return QObject::tr("image does not contains \"%1\"").arg(filter); }
		if (cond && invert)
		{ return QObject::tr("image contains \"%1\"").arg(filter); }
	}

	return QString();
}

QStringList PostFilter::filter(const QMap<QString, Token> &tokens, const QStringList &filters)
{
	QStringList ret;
	for (const QString &filter : filters)
	{
		QString err = match(tokens, filter);
		if (!err.isEmpty())
			ret.append(err);
	}
	return ret;
}

QStringList PostFilter::blacklisted(const QMap<QString, Token> &tokens, const QStringList &blacklistedTags, bool invert)
{
	QStringList detected;
	for (const QString &tag : blacklistedTags)
	{
		if (!match(tokens, tag, invert).isEmpty())
			detected.append(tag);
	}
	return detected;
}