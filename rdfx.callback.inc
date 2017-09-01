<?php

function rdfx_date_yyyy_mm_dd($date) {
  return date("Y-m-d", $date);
}

function rdfx_date_yyyy($date) {
  return date("Y", $date);
}

// Function to generate NULL value in case there is at least one forced property
function get_null($whatever) {
  return NULL;
}