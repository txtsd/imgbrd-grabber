#include "tag-loader.h"
#include "ui_tag-loader.h"
#include <QMessageBox>
#include "models/profile.h"
#include "models/site.h"
#include "models/api.h"
#include "tags/tag-api.h"
#include "tags/tag.h"
#include "helpers.h"


TagLoader::TagLoader(Profile *profile, QMap<QString, Site *> sites, QWidget *parent)
	: QDialog(parent), ui(new Ui::TagLoader), m_profile(profile), m_sites(sites)
{
	ui->setupUi(this);

	ui->comboSource->addItems(m_sites.keys());
	ui->widgetProgress->hide();

	resize(size().width(), 0);
}

TagLoader::~TagLoader()
{
	delete ui;
}

void TagLoader::cancel()
{
	emit rejected();
	close();
	deleteLater();
}

void TagLoader::start()
{
	// Get site and API
	Site *site = m_sites.value(ui->comboSource->currentText());
	Api *api = Q_NULLPTR;
	for (Api *a : site->getApis())
	{
		if (a->contains("Urls/TagApi"))
		{
			api = a;
			break;
		}
	}
	if (api == Q_NULLPTR)
	{
		error(this, tr("No API supporting tag fetching found"));
		return;
	}
	site->tagDatabase()->load();

	ui->buttonStart->setEnabled(false);

	// Show progress bar
	ui->progressBar->setValue(0);
	ui->progressBar->setMinimum(0);
	ui->progressBar->setMaximum(0);
	ui->labelProgress->setText("0");
	ui->widgetProgress->show();

	// Load all tags
	QList<Tag> allTags;
	QList<Tag> tags;
	int page = 1;
	while (!tags.isEmpty() || page == 1)
	{
		// Load tags for the current page
		QEventLoop loop;
		TagApi *tagApi = new TagApi(m_profile, site, api, page, 500, this);
		connect(tagApi, &TagApi::finishedLoading, &loop, &QEventLoop::quit);
		tagApi->load();
		loop.exec();

		tags = tagApi->tags();
		allTags.append(tags);
		tagApi->deleteLater();

		ui->progressBar->setValue(page);
		ui->labelProgress->setText(QString::number(page));
		page++;
	}

	// Update tag database
	site->tagDatabase()->setTags(allTags);
	site->tagDatabase()->save();

	// Hide progress bar
	ui->buttonStart->setEnabled(true);
	ui->widgetProgress->hide();
	resize(size().width(), 0);

	QMessageBox::information(this, tr("Finished"), tr("%n tag(s) loaded", "", allTags.count()));
}
