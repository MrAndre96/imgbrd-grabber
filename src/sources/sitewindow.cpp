#include "sitewindow.h"
#include "ui_sitewindow.h"
#include <QFile>
#include "mainwindow.h"
#include "functions.h"
#include "models/source.h"
#include "models/source-guesser.h"

extern mainWindow *_mainwindow;



siteWindow::siteWindow(QMap<QString ,Site*> *sites, QWidget *parent)
	: QDialog(parent), ui(new Ui::siteWindow), m_sites(sites)
{
	setAttribute(Qt::WA_DeleteOnClose);
	ui->setupUi(this);

	ui->progressBar->hide();
	ui->comboBox->setDisabled(true);
	ui->checkBox->setChecked(true);

	m_sources = Source::getAllSources(nullptr);
	for (Source *source : *m_sources)
	{
		ui->comboBox->addItem(QIcon(savePath("sites/" + source->getName() + "/icon.png")), source->getName());
	}
}

siteWindow::~siteWindow()
{
	delete ui;
}

void siteWindow::accept()
{
	ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

	m_url = ui->lineEdit->text();

	if (ui->checkBox->isChecked())
	{
		ui->progressBar->setValue(0);
		ui->progressBar->setMaximum(m_sources->count());
		ui->progressBar->show();

		SourceGuesser sourceGuesser(m_url, *m_sources);
		connect(&sourceGuesser, &SourceGuesser::progress, ui->progressBar, &QProgressBar::setValue);
		connect(&sourceGuesser, &SourceGuesser::finished, this, &siteWindow::finish);
		sourceGuesser.start();

		return;
	}

	Source *src = nullptr;
	for (Source *source : *m_sources)
	{
		if (source->getName() == ui->comboBox->currentText())
		{
			src = source;
			break;
		}
	}
	finish(src);
}

void siteWindow::finish(Source *src)
{
	if (ui->checkBox->isChecked())
	{
		ui->progressBar->hide();

		if (src == nullptr)
		{
			error(this, tr("Impossible de deviner le type du site. Êtes-vous sûr de l'url ?"));
			ui->comboBox->setDisabled(false);
			ui->checkBox->setChecked(false);
			ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
			ui->progressBar->hide();
			return;
		}
	}

	Site *site = new Site(m_url, src);
	src->getSites().append(site);
	m_sites->insert(site->url(), site);

	// Save new sites
	QFile f(src->getPath() + "/sites.txt");
	f.open(QIODevice::ReadOnly);
		QString sites = f.readAll();
	f.close();
	sites.replace("\r\n", "\n").replace("\r", "\n").replace("\n", "\r\n");
	QStringList stes = sites.split("\r\n", QString::SkipEmptyParts);
	stes.append(site->url());
	stes.removeDuplicates();
	stes.sort();
	f.open(QIODevice::WriteOnly);
		f.write(stes.join("\r\n").toLatin1());
	f.close();

	_mainwindow->loadSites();

	emit accepted();
	close();
}
