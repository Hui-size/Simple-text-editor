#include "aboutdialog.h"
#include "ui_aboutdialog.h"

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent), ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    ui->versionLabel->setText(tr("版本 1.0  |  Qt %1").arg(QString::fromLatin1(qVersion())));
    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
