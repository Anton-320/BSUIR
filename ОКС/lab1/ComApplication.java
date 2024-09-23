package com.byshnev.comport;

import javafx.application.Application;
import javafx.fxml.FXMLLoader;
import javafx.scene.Scene;
import javafx.stage.Stage;

import java.io.IOException;

public class ComApplication extends Application {
  @Override
  public void start(Stage stage) throws IOException {
    FXMLLoader fxmlLoader = new FXMLLoader(ComApplication.class.getResource("app-view.fxml"));
    Scene scene = new Scene(fxmlLoader.load(), 500, 700);
    stage.setTitle("Лабораторная №1");
    stage.setResizable(false);
    stage.setScene(scene);
    stage.show();
  }

  public static void main(String[] args) {
    launch();
  }
}